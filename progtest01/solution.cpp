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

	Problem() = default;

	Problem(ACustomer cust, AOrderList orders) : customer( cust ), orderList( std::move( orders ) ) {}

	Problem & operator=(const Problem & prob) {
		this->customer = prob.customer;
		this->orderList = prob.orderList;
		return *this;
	}

	Problem(const Problem & prob) {
		this->customer = prob.customer;
		this->orderList = prob.orderList;
	}
};

class MaterialInfo {
public:
	APriceList priceList;
	unsigned counter;

};

class CWeldingCompany {
public:
	static void SeqSolve(APriceList priceList, COrder & order);
	void AddProducer(AProducer prod);
	void AddCustomer(ACustomer cust);
	void AddPriceList(AProducer prod, APriceList priceList);
	void Start(unsigned thrCount);
	void Stop();
	APriceList CheckForPriceList(unsigned materialID);
	static void serveCustomerThread(CWeldingCompany & company, const ACustomer & cust);
	static void solveProblemsFromBuffer(CWeldingCompany & company);

private:
	map<unsigned, MaterialInfo> mPriceLists;

	vector<AProducer> mProducers;
	vector<ACustomer> mCustomers;

	vector<Problem> buffer;
	vector<pair<unsigned, unsigned long long>> mPriceListsTimers;

	vector<thread> mWorkers;
	vector<thread> mCustomerThreads;

    long long int activeCustomers = -1;
	mutex mtx_stop, mtx_buffer, mtx_priceList, mtx_activeCustomers;
	condition_variable cv_customer;
};

/* static */ void CWeldingCompany::SeqSolve(APriceList priceList, COrder & order) {
	vector<COrder> orderVector{order};
	ProgtestSolver( orderVector, move( priceList ) );
	order = orderVector.front();
}

void CWeldingCompany::AddProducer(AProducer prod) {
	mProducers.push_back( prod );
}

void CWeldingCompany::AddCustomer(ACustomer cust) {
	mCustomers.push_back( cust );
}

void CWeldingCompany::AddPriceList(AProducer prod, APriceList priceList) {
	// TODO: AddPriceList -
	cv_customer.notify_all();

}

void CWeldingCompany::Start(unsigned thrCount) {
	// TODO: vlakna pro customery zacnou vybirat ulohy a rikat si o priceListy
	mWorkers.reserve( mCustomers.size() );
	mCustomerThreads.reserve( mCustomers.size() );
	activeCustomers = static_cast<long long int>(mCustomers.size());

	for ( unsigned int i = 0 ; i < thrCount ; ++i ) {
		mWorkers.emplace_back( solveProblemsFromBuffer, this );
	}

	for ( auto & mCustomer : mCustomers ) {
		mCustomerThreads.emplace_back( serveCustomerThread, this, ref( mCustomer ) );
	}

	// TODO: worker vlakna zacnou tahat z bufferu ulohy, pocitat je a vracet


}

APriceList CWeldingCompany::CheckForPriceList(unsigned materialID) {
	unique_lock<mutex> lock( mtx_priceList );
	auto info = mPriceLists.find( materialID );
	if ( info != mPriceLists.end() ) {
		return mPriceLists.at( materialID ).priceList;
	}
	lock.unlock();
	// TODO: bloknout, dokud se nevrati PriceList od všech
	for ( auto & producer : mProducers ) {
		producer->SendPriceList( materialID );
	}

	// TODO: create new priceList for this material, add it to the list and return it
	return nullptr;
}

void CWeldingCompany::Stop() {
	{
		unique_lock<mutex> ul( mtx_stop );
	}

	for ( auto & t : mWorkers ) {
		t.join();
	}

	for ( auto & t : mCustomerThreads ) {
		t.join();
	}

}

void CWeldingCompany::serveCustomerThread(CWeldingCompany & company, const ACustomer & cust) {
	while ( true ) {
		AOrderList orderList = cust->WaitForDemand();
		if ( orderList->m_List.empty() ) {
			unique_lock<mutex> lock( company.mtx_activeCustomers );
			company.activeCustomers--;
			break;
		}

		company.CheckForPriceList( orderList->m_MaterialID );

		unique_lock<mutex> lock( company.mtx_priceList );
		company.cv_customer.wait( lock, company.mPriceLists[orderList->m_MaterialID].counter != 0 );

		{
			unique_lock<mutex> lock( company.mtx_buffer );
			company.buffer.emplace_back( Problem( cust, orderList ) );
		}


	}
}

void CWeldingCompany::solveProblemsFromBuffer(CWeldingCompany & company) {
	// TODO!! workers
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
	test.AddCustomer( make_shared<CCustomerTest>( 2 ) );
	p2->Start();
	test.Start( 3 );
	test.Stop();
	p2->Stop();
	return 0;
}

#endif /* __PROGTEST__ */
