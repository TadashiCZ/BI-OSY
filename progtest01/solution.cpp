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

class Package {
public:
	ACustomer customer;
	AOrderList orderList;
};

class Material {
public:
	APriceList priceList;
	set<AProducer> producers;
};

class CWeldingCompany {
public:
	static void SeqSolve( APriceList priceList, COrder & order );
	void AddProducer( const AProducer & prod );
	void AddCustomer( const ACustomer & cust );
	void AddPriceList( AProducer prod, APriceList priceList );
	void Start( unsigned thrCount );
	void Stop( void );
	void customerThread( ACustomer cust );
	void workerThread();

	map<unsigned, Material> mPriceList;
	queue<Package> mBuffer;
	vector<AProducer> mProducers;
	vector<ACustomer> mCustomers;

	vector<thread> mWorkerThreads;
	vector<thread> mCustomerThreads;

	unsigned mThrCount;
	unsigned long long int mDoneCustomers = 0;
	unsigned bufferLimit = 10;

	mutex mtx_Buffer;
	mutex mtx_PriceList;
	mutex mtx_Customers;

	condition_variable cv_BufferEmpty;
	condition_variable cv_BufferFull;
	condition_variable cv_PriceListExists;
	condition_variable cv_PriceListFull;

};

/* static */ void CWeldingCompany::SeqSolve( APriceList priceList, COrder & order ) {
	vector<COrder> orderVector{order};
	ProgtestSolver( orderVector, move( priceList ) );
	order = orderVector.front();
}

void CWeldingCompany::AddProducer( const AProducer & prod ) {
	mProducers.push_back( prod );
}

void CWeldingCompany::AddCustomer( const ACustomer & cust ) {
	mCustomers.push_back( cust );
}

void CWeldingCompany::Start( unsigned thrCount ) {
	mThrCount = thrCount;

	for ( auto & mCustomer : mCustomers ) {
		mCustomerThreads.emplace_back( &CWeldingCompany::customerThread, this, ref( mCustomer ) );
	}

	for ( unsigned i = 0 ; i < mThrCount ; ++i ) {
		mWorkerThreads.emplace_back( &CWeldingCompany::workerThread, this );
	}

	//cout << "START: WORKERS: " << thrCount << ", CUSTOMERS: " << mCustomers.size() << endl;
}

void CWeldingCompany::AddPriceList( AProducer prod, APriceList priceList ) {
	unsigned matID = priceList->m_MaterialID;
	{
		unique_lock<mutex> lockExists( mtx_PriceList );
		auto it = mPriceList.find( matID );

		if ( it == mPriceList.end() ) {
			//	cout << this_thread::get_id() << ":ADD_PRICE_LIST: priceList for ID: " << matID << " not found\n";
			make_shared<CPriceList>( matID );
			Material newMat;
			newMat.priceList = priceList;
			newMat.producers.insert( prod );
			//cout << this_thread::get_id() << ":ADD_PRICE_LIST: new producer in priceList (now " << newMat.producers.size() << ")\n";
			mPriceList.insert( pair<unsigned, Material>( matID, newMat ) );
			cv_PriceListExists.notify_all();
			cv_PriceListFull.notify_all();
			return;
		}
	}
	//cout << this_thread::get_id() << ":ADD_PRICE_LIST: material with ID: " << priceList->m_MaterialID	     << " already in priceList\n";
	cv_PriceListExists.notify_all();
	{
		unique_lock<mutex> lockExists( mtx_PriceList );

		if ( mPriceList.at( matID ).producers.count( prod ) != 0 ) {
			//cout << this_thread::get_id() << ":ADD_PRICE_LIST: priceList for this producer already exists\n";
			return;
		}

		mPriceList.at( matID ).producers.insert( prod );
		//cout << this_thread::get_id() << ":ADD_PRICE_LIST: new producer in priceList (now " << mPriceList.at( matID ).producers.size() << ")\n";
		for ( auto itNew = priceList->m_List.begin() ; itNew != priceList->m_List.end() ; ++itNew ) {
			for ( auto itOld = mPriceList.at( priceList->m_MaterialID ).priceList->m_List.begin() ;  itOld !=  mPriceList.at( priceList->m_MaterialID ).priceList->m_List.end() ; ++itOld ) {
				if ( ( itNew->m_W == itOld->m_W && itNew->m_H == itOld->m_H ) ||
				     ( itNew->m_W == itOld->m_H && itNew->m_H == itOld->m_W ) ) {
					if ( itOld->m_Cost <= itNew->m_Cost ) {
						continue;
					} else {
						itOld->m_Cost = itNew->m_Cost;
						continue;
					}
				}
			}
			mPriceList[priceList->m_MaterialID].priceList->m_List.push_back( *itNew );
		}


	}
//	cout << this_thread::get_id() << ":ADD_PRICE_LIST: notifying about full priceLists\n";
	cv_PriceListFull.notify_all();
//	cout << this_thread::get_id() << ":ADD_PRICE_LIST: END OF FUNCTION\n";

}

void CWeldingCompany::customerThread( ACustomer cust ) {
	//cout << this_thread::get_id() << ":CUST: START\n";
	while ( true ) {
		AOrderList orderList = cust->WaitForDemand();
		if ( orderList.get() == nullptr ) {
			//	cout << this_thread::get_id() << ":CUST: stop customer thread, customer finished\n";
			{
				unique_lock<mutex> lockCust( mtx_Customers );
				++mDoneCustomers;
			}
			break;
		}

		Package pack = Package();
		pack.orderList = orderList;
		pack.customer = cust;

		for ( auto & mProducer : mProducers ) {
			mProducer->SendPriceList( pack.orderList->m_MaterialID );
		}

		{
			unique_lock<mutex> lockFull( mtx_Buffer );
			cv_BufferFull.wait( lockFull, [&] { return mBuffer.size() < bufferLimit; } );
			//cout << this_thread::get_id() << ":CUST: push package into buffer\n";
			mBuffer.push( pack );
		}

		//cout << this_thread::get_id() << ":CUST: notyfing about added package\n";
		cv_BufferEmpty.notify_all();
	}
//	cout << this_thread::get_id() << ":CUST: END OF FUNCTION\n";
}

void CWeldingCompany::workerThread() {
//	cout << this_thread::get_id() << ":WORK: START\n";
	while ( true ) {
		{
			unique_lock<mutex> lock( mtx_Buffer );
			if ( mBuffer.empty() && mDoneCustomers == mCustomers.size() ) {
				//	cout << this_thread::get_id() << ":WORK: stop worker thread, all customers finished, empty buffer\n";
				cv_BufferEmpty.notify_all();
				break;
			}
		}

		{
			unique_lock<mutex> lockEmpty( mtx_Buffer );
			cv_BufferEmpty.wait( lockEmpty, [&] {
				//		cout << this_thread::get_id() << ":WORK: waiting with empty buffer\n";
				return !( mBuffer.empty() && mDoneCustomers != mCustomers.size() );
			} );
		}

		Package pack;
		{
			unique_lock<mutex> lockEmpty( mtx_Buffer );
			if ( mBuffer.empty() ) {
				continue;
			}
			pack = mBuffer.front();
			mBuffer.pop();
		}

		cv_BufferFull.notify_one();

		for ( auto & producer : mProducers ) {
			producer->SendPriceList( pack.orderList->m_MaterialID );
		}

		{
			unique_lock<mutex> lockExist( mtx_PriceList );
			cv_PriceListExists.wait( lockExist, [&] {
				//		cout << this_thread::get_id() << ":WORK: waiting for priceList existence\n";
				return mPriceList.find( pack.orderList->m_MaterialID ) != mPriceList.end();
			} );
		}

		{
			unique_lock<mutex> lockFull( mtx_PriceList );
			cv_PriceListFull.wait( lockFull, [&] {
				//	cout << this_thread::get_id() << ":WORK: waiting for priceList filling: producers: " << mProducers.size() << ", prodSize: " << mPriceList.at( pack.orderList->m_MaterialID ).producers.size() << "\n";
				return mPriceList.at( pack.orderList->m_MaterialID ).producers.size() >= mProducers.size();
			} );
		}

		Material material;
		{
			unique_lock<mutex> lockPrice( mtx_PriceList );
			material = mPriceList.at( pack.orderList->m_MaterialID );
		}

		//	cout << this_thread::get_id() << ":WORK: solver called\n";
		ProgtestSolver( pack.orderList->m_List, material.priceList );
		//	cout << this_thread::get_id() << ":WORK: solver finished\n";


		pack.customer->Completed( pack.orderList );

		//	cout << this_thread::get_id() << ":WORK: order with ID " << pack.orderList->m_MaterialID << " completed\n";
	}

//	cout << this_thread::get_id() << ":WORK: END OF FUNCTION\n";
}

void CWeldingCompany::Stop() {
	for ( unsigned i = 0 ; i < mCustomers.size() ; ++i ) {
		mCustomerThreads[i].join();
	}

	cv_BufferEmpty.notify_all();
	cv_PriceListFull.notify_all();
	cv_BufferFull.notify_all();
	cv_PriceListExists.notify_all();

	for ( unsigned j = 0 ; j < mThrCount ; ++j ) {
		mWorkerThreads[j].join();
	}

}

//-------------------------------------------------------------------------------------------------
#ifndef __PROGTEST__

int main( void ) {
	using namespace std::placeholders;
	CWeldingCompany test;

	AProducer p1 = make_shared<CProducerSync>( bind( &CWeldingCompany::AddPriceList, &test, _1, _2 ) );
	AProducerAsync p2 = make_shared<CProducerAsync>( bind( &CWeldingCompany::AddPriceList, &test, _1, _2 ) );
	test.AddProducer( p1 );
	test.AddProducer( p2 );
	test.AddCustomer( make_shared<CCustomerTest>( 2 ) );
	p2->Start();
	test.Start( 3 );
	test.Stop();
	p2->Stop();
	return 0;
}

#endif /* __PROGTEST__ */
