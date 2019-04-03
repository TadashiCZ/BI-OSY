#include <utility>

#include <utility>

#include <utility>

#ifndef __PROGTEST__

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cfloat>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "progtest_solver.h"
#include "sample_tester.h"

using namespace std;
#endif /* __PROGTEST__ */

class Problem {
public:
	ACustomer customer;
	AOrderList orderList;

	Problem() : customer( nullptr ), orderList( nullptr ) {}

	Problem( ACustomer cust, AOrderList ordeL ) : customer( std::move( cust ) ), orderList( std::move( ordeL ) ) {}

	Problem & operator=( const Problem & prob ) = default;

	Problem( const Problem & prob ) {
		this->customer = prob.customer;
		this->orderList = prob.orderList;
	}
};

class MaterialInfo {
public:
	MaterialInfo() {
		counter = 99999999999;
	}

	MaterialInfo( const MaterialInfo & mater ) {
		counter = mater.counter;
		priceList = mater.priceList;
	}

	MaterialInfo( unsigned long cnt, APriceList pL ) : counter( cnt ), priceList( std::move( pL ) ) {}

	unsigned long counter;
	APriceList priceList;


};

class CWeldingCompany {
public:
	static void SeqSolve( APriceList priceList, COrder & order );
	void AddProducer( AProducer prod );
	void AddCustomer( ACustomer cust );
	void AddPriceList( AProducer prod, APriceList priceList );
	void Start( unsigned thrCount );
	void Stop();
	void customerThreadFunction( ACustomer & cust );
	void workerThreadFunction();
	APriceList checkForPriceList( unsigned materialID );
private:
	map<unsigned, MaterialInfo> mPriceLists;
	mutex mtx_priceList;

	vector<AProducer> mProducers;
	vector<ACustomer> mCustomers;

	queue<Problem> mBuffer;
	mutex mtx_buffer;
	condition_variable cv_buffer;

	vector<thread> mWorkers;
	vector<thread> mCustomerThreads;

	long long int mActiveCustomers = -1;
	mutex mtx_activeCustomers;
	condition_variable cv_activeCustomers;

	unsigned mThrCount = 0;

	condition_variable cv_PriceListQueue;
	mutex mtx_priceListQueue;
};

/* static */ void CWeldingCompany::SeqSolve( APriceList priceList, COrder & order ) {
	vector<COrder> orderVector{order};
	ProgtestSolver( orderVector, move( priceList ) );
	order = orderVector.front();
}

void CWeldingCompany::AddProducer( AProducer prod ) {
	mProducers.push_back( prod );
}

void CWeldingCompany::AddCustomer( ACustomer cust ) {
	mCustomers.push_back( cust );
}

void CWeldingCompany::AddPriceList( AProducer prod, APriceList priceList ) {
	AProducer producer = prod;
	{
		unique_lock<mutex>(mtx_priceList);

		if ( mPriceLists.at( priceList->m_MaterialID ).priceList == nullptr ) {
			cout << "ERROR: No priceList" << endl;
			return;
		}

		for ( auto itAlready = mPriceLists.at( priceList->m_MaterialID ).priceList->m_List.begin() ;
		      itAlready != mPriceLists.at( priceList->m_MaterialID ).priceList->m_List.end() ; ++itAlready ) {
			for ( auto itNotYet = priceList->m_List.begin() ; itNotYet != priceList->m_List.end() ; ++itNotYet ) {

				if ( ( itAlready->m_H == itNotYet->m_H && itAlready->m_W == itNotYet->m_W ) ||
				     ( itAlready->m_W == itNotYet->m_H && itAlready->m_H == itNotYet->m_W ) ) {
					if ( itAlready->m_Cost <= itNotYet->m_Cost ) {
						priceList->m_List.erase( itNotYet );
						continue;
					} else {
						itAlready->m_Cost = itNotYet->m_Cost;
						priceList->m_List.erase( itNotYet );
					}
				}
			}
		}

		mPriceLists.at( priceList->m_MaterialID ).priceList->m_List.insert(
				mPriceLists.at( priceList->m_MaterialID ).priceList->m_List.end(), priceList->m_List.begin(),
				priceList->m_List.end() );

		mPriceLists.at( mPriceLists.at( priceList->m_MaterialID ).priceList->m_MaterialID ).counter--;
		cv_PriceListQueue.notify_all();
	}
}

void CWeldingCompany::Start( unsigned thrCount ) {
	// init part
	mActiveCustomers = mCustomers.size();

	this->mThrCount = thrCount;

	mWorkers.reserve( this->mThrCount );
	mCustomerThreads.reserve( mCustomers.size() );

	for ( auto & mCustomer : mCustomers ) {
		mCustomerThreads.emplace_back( &CWeldingCompany::customerThreadFunction, this, ref( mCustomer ) );
	}

	for ( unsigned i = 0 ; i < thrCount ; ++i ) {
		mWorkers.emplace_back( &CWeldingCompany::workerThreadFunction, this );
	}

}

APriceList CWeldingCompany::checkForPriceList( unsigned materialID ) {
	if ( mPriceLists.find( materialID ) != mPriceLists.end() ) {
		return mPriceLists.at( materialID ).priceList;
	} else {
		// insert empty and init counter
		mPriceLists.insert(
				pair<unsigned, MaterialInfo>( materialID,
				                              MaterialInfo( mCustomers.size(), make_shared<CPriceList>( materialID ) ) ) );

		// call for priceLists
		for ( auto & producer :  mProducers ) {
			producer->SendPriceList( materialID );
		}

		unique_lock<mutex> lock( mtx_priceListQueue );
		cv_PriceListQueue.wait( lock, [&] { return mPriceLists[materialID].counter != 0; } );
	}

	return mPriceLists[materialID].priceList;

}

void CWeldingCompany::Stop() {
	{
		unique_lock<mutex> lock( mtx_activeCustomers );
		cv_activeCustomers.wait( lock, [&] { return mActiveCustomers == 0; } );
	}

	for ( unsigned i = 0 ; i < mThrCount ; ++i ) {
		cv_buffer.notify_all();
	}

	for ( auto & worker : mWorkers ) {
		worker.join();
	}

	for ( auto & customer : mCustomerThreads ) {
		customer.join();
	}

}

void CWeldingCompany::customerThreadFunction( ACustomer & cust ) {
	cout << "Start customerThreadFunction\n";
	while ( true ) {
		// get orderList
		AOrderList orderList = cust->WaitForDemand();
		if ( orderList.get() == nullptr ) {
			{
				unique_lock<mutex> lock( mtx_activeCustomers );
				cv_activeCustomers.notify_all();
				mActiveCustomers--;
			}
			break;
		}
		// get priceList
		APriceList priceList = checkForPriceList( orderList->m_MaterialID );

		//create Problem and put it into Buffer
		Problem prob = Problem( cust, orderList );

		cout << "Put problem into buffer:\nOrders:\n";
		for ( unsigned i = 0 ; i < orderList->m_List.size() ; ++i ) {
			cout << "m_W: " << orderList->m_List[i].m_W << ", m_H: " << orderList->m_List[i].m_H << ", m_S: "
			     << orderList->m_List[i].m_WeldingStrength << "\n";
		}

		{
			unique_lock<mutex> lock( mtx_buffer );
			mBuffer.push( prob );
		}

		cv_buffer.notify_all();
	}

}

void CWeldingCompany::workerThreadFunction() {
	cout << "Start workerThreadFunction\n";
	while ( true ) {

		{
			unique_lock<mutex> lock1( mtx_activeCustomers );
			unique_lock<mutex> lock2( mtx_buffer );
			if ( mActiveCustomers == 0 && mBuffer.empty() ) {
				break;
			}
		}


		{
			unique_lock<mutex> lock( mtx_buffer );
			if ( mBuffer.empty() ) {
				continue;
			}
		}

		Problem prob;
		{
			unique_lock<mutex> lock( mtx_buffer );
			prob = mBuffer.front();
			mBuffer.pop();
		}

		APriceList priceList;

		{
			unique_lock<mutex> lock_price( mtx_priceList );
			priceList = mPriceLists[prob.orderList->m_MaterialID].priceList;
		}

		cout << "PriceList " << priceList->m_MaterialID << ":\n";
		if ( priceList->m_List.empty() ) {
			cout << "empty price list\n";
		} else {
			for ( unsigned j = 0 ; j < priceList->m_List.size() ; ++j ) {
				cout << "m_W: " << priceList->m_List[j].m_W << ", m_H: " << priceList->m_List[j].m_H << ", m_Cost: "
				     << priceList->m_List[j].m_Cost << "\n";
			}
			cout << "\n\n";
		}
		cout << "Call solver\n";
		ProgtestSolver( prob.orderList->m_List, priceList );
		cout << "Solver result: MaterialID: " << prob.orderList->m_MaterialID << ", prices:" << endl;
		for ( unsigned i = 0 ; i < prob.orderList->m_List.size() ; ++i ) {
			cout << "Order " << i << ": " << prob.orderList->m_List[i].m_Cost << endl;
		}
		cout << endl;
		prob.customer->Completed( prob.orderList );
	}
}

//-------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__

int main() {
	using namespace std::placeholders;
	CWeldingCompany test;

	AProducer p1 = make_shared<CProducerSync>( bind( &CWeldingCompany::AddPriceList, &test, _1, _2 ) );
	AProducerAsync p2 = make_shared<CProducerAsync>( bind( &CWeldingCompany::AddPriceList, &test, _1, _2 ) );
	test.AddProducer( p1 );
	test.AddProducer( p2 );
	test.AddCustomer( make_shared<CCustomerTest>( 1 ) );
	p2->Start();
	test.Start( 3 );
	test.Stop();
	p2->Stop();
	return 0;
}

#endif /* __PROGTEST__ */
