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
	void customerThreadFunction( const ACustomer & cust );
	void workerThreadFunction();
	APriceList checkForPriceList( unsigned materialID );

	bool compareProducts( CProd & oldProd, CProd & newProd ) {
		return ( ( oldProd.m_H == newProd.m_H && oldProd.m_W == newProd.m_W ) ||
		         ( oldProd.m_W == newProd.m_H && oldProd.m_H == newProd.m_W ) );
	}

private:
	map<unsigned, MaterialInfo> mPriceLists;
	mutex mtx_priceList;
	condition_variable cv_priceListExists;
	condition_variable cv_priceListIsFull;

	vector<AProducer> mProducers;
	vector<ACustomer> mCustomers;

	queue<Problem> mBuffer;
	mutex mtx_buffer;
	condition_variable cv_buffer;
	condition_variable cv_bufferEmpty;

	vector<thread> mWorkers;
	vector<thread> mCustomerThreads;

	long long int mActiveCustomers = -1;
	mutex mtx_activeCustomers;
	condition_variable cv_activeCustomers;

	unsigned mThrCount = 0;


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
		unique_lock<mutex> lock( mtx_priceList );

		auto it = mPriceLists.find( priceList->m_MaterialID );

		if ( it == mPriceLists.end() ) {
			mPriceLists.insert( pair<unsigned, MaterialInfo>(
					priceList->m_MaterialID,
					MaterialInfo( mCustomers.size(), make_shared<CPriceList>( priceList->m_MaterialID ) ) ) );
		}

		cv_priceListExists.notify_all();
		MaterialInfo matInfo = mPriceLists.at( priceList->m_MaterialID );

		for ( auto itNotYet = priceList->m_List.begin() ; itNotYet != priceList->m_List.end() ; ++itNotYet ) {
			for ( auto & itAlready : matInfo.priceList->m_List ) {
				if ( compareProducts( itAlready, *itNotYet ) ) {
					if ( itAlready.m_Cost <= itNotYet->m_Cost ) {
						continue;
					} else {
						itAlready.m_Cost = itNotYet->m_Cost;
						continue;
					}
				}
			}
			mPriceLists[priceList->m_MaterialID].priceList->m_List.push_back( *itNotYet );
		}

		mPriceLists.at( priceList->m_MaterialID ).counter--;
		cv_priceListIsFull.notify_all();
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
	unique_lock<mutex> lock( mtx_priceList );
	if ( mPriceLists.find( materialID ) != mPriceLists.end() ) {
		if ( mPriceLists.at( materialID ).counter == 0 ) {
			return mPriceLists.at( materialID ).priceList;
		}
	} else {
		cv_priceListExists.wait( lock, [&] { return mPriceLists.find( materialID ) != mPriceLists.end(); } );
		cv_priceListIsFull.wait( lock, [&] { return mPriceLists[materialID].counter == 0; } );
	}

	return mPriceLists.at( materialID ).priceList;

}

void CWeldingCompany::Stop() {
	{
		unique_lock<mutex> lock( mtx_activeCustomers );
		cv_activeCustomers.wait( lock, [&] { return mActiveCustomers == 0; } );
	}

	for ( auto & customer : mCustomerThreads ) {
		customer.join();
	}

	for ( unsigned i = 0 ; i < mThrCount ; ++i ) {
		cv_bufferEmpty.notify_all();
		cv_buffer.notify_all();

	}

	for ( auto & worker : mWorkers ) {
		worker.join();
	}


}

void CWeldingCompany::customerThreadFunction( const ACustomer & cust ) {
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

		for ( auto & producer :  mProducers ) {
			producer->SendPriceList( orderList->m_MaterialID );
		}

		//create Problem and put it into Buffer
		Problem prob = Problem( cust, orderList );
/*
		cout << "Put problem into buffer:\nOrders:\n";
		for ( unsigned i = 0 ; i < orderList->m_List.size() ; ++i ) {
			cout << "m_W: " << orderList->m_List[i].m_W << ", m_H: " << orderList->m_List[i].m_H << ", m_S: "
			     << orderList->m_List[i].m_WeldingStrength << "\n";
		}*/

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

		Problem prob;
		{
			unique_lock<mutex> lock( mtx_buffer );
			cv_bufferEmpty.wait( lock, [&] { return !mBuffer.empty(); } );
		}


		{
			unique_lock<mutex> lock( mtx_buffer );
			if ( !mBuffer.empty() ) {
				prob = mBuffer.front();
				mBuffer.pop();
			}
		}

		APriceList priceList = checkForPriceList( prob.orderList->m_MaterialID );

		/*cout << "PriceList " << priceList->m_MaterialID << ":\n";
		if ( priceList->m_List.empty() ) {
			cout << "empty price list\n";
		} else {
			for ( unsigned j = 0 ; j < priceList->m_List.size() ; ++j ) {
				cout << "m_W: " << priceList->m_List[j].m_W << ", m_H: " << priceList->m_List[j].m_H << ", m_Cost: "
				     << priceList->m_List[j].m_Cost << "\n";
			}
			cout << "\n\n";
		}
		cout << "Call solver\n";*/
		ProgtestSolver( prob.orderList->m_List, priceList );
		/*cout << "Solver result: MaterialID: " << prob.orderList->m_MaterialID << ", prices:" << endl;
		for ( unsigned i = 0 ; i < prob.orderList->m_List.size() ; ++i ) {
			cout << "Order " << i << ": " << prob.orderList->m_List[i].m_Cost << endl;
		}
		cout << endl;*/
		prob.customer->Completed( prob.orderList );
	}
}

//-------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__

int main() {
	using namespace std::placeholders;
	CWeldingCompany test, test2;
/*

	AProducer p1 = make_shared<CProducerSync>( bind( &CWeldingCompany::AddPriceList, &test, _1, _2 ) );
	AProducerAsync p2 = make_shared<CProducerAsync>( bind( &CWeldingCompany::AddPriceList, &test, _1, _2 ) );
	test.AddProducer( p1 );
	test.AddProducer( p2 );
	test.AddCustomer( make_shared<CCustomerTest>( 1 ) );
	p2->Start();
	test.Start( 3 );
	test.Stop();
	p2->Stop();
*/


	AProducer p3 = make_shared<CProducerSync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	AProducer p6 = make_shared<CProducerSync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	AProducer p4 = make_shared<CProducerSync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	AProducer p5 = make_shared<CProducerSync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	test2.AddProducer( p3 );
	test2.AddProducer( p4 );
	test2.AddProducer( p5 );
	test2.AddProducer( p6 );
	AProducer p7 = make_shared<CProducerSync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	AProducerAsync p8 = make_shared<CProducerAsync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	AProducerAsync p9 = make_shared<CProducerAsync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	AProducerAsync p10 = make_shared<CProducerAsync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	AProducerAsync p11 = make_shared<CProducerAsync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );

	AProducerAsync p12 = make_shared<CProducerAsync>( bind( &CWeldingCompany::AddPriceList, &test2, _1, _2 ) );
	test2.AddProducer( p9 );
	test2.AddProducer( p10 );
	test2.AddProducer( p7 );
	test2.AddProducer( p11 );
	test2.AddProducer( p12 );
	test2.AddProducer( p8 );
	test2.AddCustomer( make_shared<CCustomerTest>( 20 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 4 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 24 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 9 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 20 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 8 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 24 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 9 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 20 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 4 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 24 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 9 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 4 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 24 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 9 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 20 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 8 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 24 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 9 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 20 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 4 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 24 ) );
	test2.AddCustomer( make_shared<CCustomerTest>( 9 ) );
	p8->Start();
	p9->Start();
	p10->Start();
	p11->Start();
	p12->Start();
	test2.Start( 4 );
	test.Stop();
	p12->Stop();
	p11->Stop();
	p10->Stop();
	p9->Stop();
	p8->Stop();
	cout << "FINISHED" << endl;


	return 0;
}

#endif /* __PROGTEST__ */
