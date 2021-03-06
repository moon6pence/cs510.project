#include <vector>
#include <memory>
#include <chrono>
#include <iostream>

#include <cstdio>
#include <cmath>

// Precision to use for calculations
#define fptype float
#define ERR_CHK
#define NUM_RUNS 1

const int OptionDataCount = 10000000;

typedef struct OptionData_ 
{
	fptype s;			// spot price
	fptype strike;		// strike price
	fptype r;			// risk-free interest rate
	fptype divq;		// dividend rate
	fptype v;			// volatility
	fptype t;			// time to maturity or option expiration in years 
						//     (1yr = 1.0, 6mos = 0.5, 3mos = 0.25, ..., etc)  
	char   OptionType;	// Option type.  "P"=PUT, "C"=CALL
	fptype divs;		// dividend vals (not used in this test)
	fptype DGrefval;	// DerivaGem Reference Value
} OptionData;

OptionData data_init[] = 
{
	#include "optionData.txt"
};

////////////////////////////////////////////////////////////////////////////////
// Cumulative Normal Distribution Function
// See Hull, Section 11.8, P.243-244
#define inv_sqrt_2xPI 0.39894228040143270286

fptype CNDF ( fptype InputX ) 
{
	int sign;

	fptype OutputX;
	fptype xInput;
	fptype xNPrimeofX;
	fptype expValues;
	fptype xK2;
	fptype xK2_2, xK2_3;
	fptype xK2_4, xK2_5;
	fptype xLocal, xLocal_1;
	fptype xLocal_2, xLocal_3;

	// Check for negative value of InputX
	if (InputX < 0.0) 
	{
		InputX = -InputX;
		sign = 1;
	} 
	else
	{
		sign = 0;
	}

	xInput = InputX;
 
    // Compute NPrimeX term common to both four & six decimal accuracy calcs
	expValues = exp(-0.5f * InputX * InputX);
	xNPrimeofX = expValues;
	xNPrimeofX = xNPrimeofX * inv_sqrt_2xPI;

	xK2 = 0.2316419 * xInput;
	xK2 = 1.0 + xK2;
	xK2 = 1.0 / xK2;
	xK2_2 = xK2 * xK2;
	xK2_3 = xK2_2 * xK2;
	xK2_4 = xK2_3 * xK2;
	xK2_5 = xK2_4 * xK2;
    
	xLocal_1 = xK2 * 0.319381530;
	xLocal_2 = xK2_2 * (-0.356563782);
	xLocal_3 = xK2_3 * 1.781477937;
	xLocal_2 = xLocal_2 + xLocal_3;
	xLocal_3 = xK2_4 * (-1.821255978);
	xLocal_2 = xLocal_2 + xLocal_3;
	xLocal_3 = xK2_5 * 1.330274429;
	xLocal_2 = xLocal_2 + xLocal_3;

	xLocal_1 = xLocal_2 + xLocal_1;
	xLocal   = xLocal_1 * xNPrimeofX;
	xLocal   = 1.0 - xLocal;

	OutputX  = xLocal;
    
	if (sign) 
	{
		OutputX = 1.0 - OutputX;
	}
    
	return OutputX;
} 


//////////////////////////////////////////////////////////////////////////////////////
fptype BlkSchlsEqEuroNoDiv( fptype sptprice,
							fptype strike, fptype rate, fptype volatility,
							fptype time, int otype, float timet )    
{
	fptype OptionPrice;

	// local private working variables for the calculation
	fptype xStockPrice;
	fptype xStrikePrice;
	fptype xRiskFreeRate;
	fptype xVolatility;
	fptype xTime;
	fptype xSqrtTime;

	fptype logValues;
	fptype xLogTerm;
	fptype xD1; 
	fptype xD2;
	fptype xPowerTerm;
	fptype xDen;
	fptype d1;
	fptype d2;
	fptype FutureValueX;
	fptype NofXd1;
	fptype NofXd2;
	fptype NegNofXd1;
	fptype NegNofXd2;    
    
	xStockPrice = sptprice;
	xStrikePrice = strike;
	xRiskFreeRate = rate;
	xVolatility = volatility;

	xTime = time;
	xSqrtTime = sqrt(xTime);

	logValues = log( sptprice / strike );
        
	xLogTerm = logValues;
        
	xPowerTerm = xVolatility * xVolatility;
	xPowerTerm = xPowerTerm * 0.5;
	xDen = xVolatility * xSqrtTime;

    xD1 = xRiskFreeRate + xPowerTerm;
	xD1 = xD1 * xTime;
	xD1 = xD1 + xLogTerm;
	xD1 = xD1 / xDen;
	xD2 = xD1 -  xDen;

	d1 = xD1;
	d2 = xD2;
    
	NofXd1 = CNDF( d1 );
	NofXd2 = CNDF( d2 );

	FutureValueX = strike * ( exp( -(rate)*(time) ) );        
	if (otype == 0) 
	{            
		OptionPrice = (sptprice * NofXd1) - (FutureValueX * NofXd2);
	} 
	else 
	{ 
		NegNofXd1 = (1.0 - NofXd1);
		NegNofXd2 = (1.0 - NofXd2);
		OptionPrice = (FutureValueX * NegNofXd2) - (sptprice * NegNofXd1);
	}
    
	return OptionPrice;
}

//////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv)
{
    // How many elements do we have in our hard-coded datafile?
    int initOptionNum =  ((sizeof(data_init)) / sizeof(OptionData));

    // Initialize option data
    OptionData *optionData = new OptionData[OptionDataCount];

    for (int i = 0; i < OptionDataCount; i++)
        optionData[i] = data_init[i % initOptionNum];

	// tic
	auto start = std::chrono::high_resolution_clock::now();

    // process
    for (int i = 0; i < OptionDataCount; i++)
    {
        for (int j = 0; j < NUM_RUNS; j++) 
        {
            fptype price = BlkSchlsEqEuroNoDiv(
                optionData[i].s, optionData[i].strike, 
                optionData[i].r, optionData[i].v, optionData[i].t, 
                optionData[i].OptionType == 'P' ? 1 : 0, 0);
#ifdef ERR_CHK 
            fptype priceDelta = optionData[i].DGrefval - price;
            if (fabs(priceDelta) >= 1e-4)
            {
                printf("Error on %d. Computed=%.5f, Ref=%.5f, Delta=%.5f\n",
                       j, price, optionData[i].DGrefval, priceDelta);
            }
#endif
        }
    }

	// toc
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed_msec = end - start;
	std::cout << "Elapsed time: " << elapsed_msec.count() << " msec" << std::endl;

    // free
    delete[] optionData;

    return 0;
}
