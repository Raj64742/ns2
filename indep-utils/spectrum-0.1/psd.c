/* psd.c - main file for spectrum

This program is free software; you can redistribute or modify it under the
terms of GNU general public license as published by the free software foundation

*/



#include "spect.h"
main(int argc,char *argv[])
{

	int i=0;
	int temp;
	int j,c,lag,flag;
	float t;
	int NoSamples;
	double *abval,*cumpsd;
	double freq,quantile_freq;
	double sum=0;
	float average=0; // Used to Calculate the Mean value of the samples
        float sd=0;  // Standard Deviation
        double sampling_frequency; // Sampling frequency
        double dif;
	int option;

	char *Usage1="Usage : ./spect -f data-file";
	char *Usage2="View README for details";
	
	extern char *optarg;
        extern int opterr;
        opterr=0;
	
	/* Input samples */
	
	struct ele *x;


	/* autocorrelation array  */

	struct ele *ax;


	/* FFTW initializations */

	fftw_complex *in,*out;
	fftw_plan p;

	/* To read the data from a file. */

	FILE *fp;  // Input samples 
	FILE *fpsd;  // Output samples
	FILE *fcpsd; // Cumulative spectrum
	FILE *fautocorr; // Autocorrelation Samples 
        FILE *statistics; // The statistical details
	
	

        // To get command line options using getopt

        if(argc == 1)
	{
	printf("%s\n%s\n",Usage1,Usage2);
	exit(0);
	}
	
	
	while(1) {
	option=getopt(argc,argv,"f:");
	if(option == -1)
	break;
        
	switch(option) {
	case 'f' :
	    argv[1]=optarg;
	    break;
	 
	default :
	   printf("Wrong option\n");
	   printf("%s\n%s\n",Usage1,Usage2);
	   exit(0);
	   }
	
          	
	} //while ends
       	
        system("rm Details");	
	
	fp=fopen(argv[1],"r"); 
	fpsd=fopen("psd","w");
	fcpsd=fopen("cpsd","w");
	fautocorr=fopen("Autocorrelation","w");
	statistics=fopen("Details","w");

	int N=0; // Number of samples
	float tim[3],temp_arr[2]; //Time Stamp
	int index=0;
	
	
	
	while(fscanf(fp,"%f %d",&tim[index],&temp) != EOF)
	{
        if(index < 2)
         {
          temp_arr[index]=tim[index];
	  ++index;
         }
	   ++N;
        }

	// To find the sampling frequency

	dif=temp_arr[1]-temp_arr[0];
        sampling_frequency=1.0/dif;
//	sampling_frequency=(double)1000;
	
	
	x=(struct ele *)malloc(N*sizeof(struct ele));  //Storage allocation for input samples 

	ax=(struct ele *)malloc((2*N-1)*sizeof(struct ele));  // Autocorrelation Samples storage

	fseek(fp,0,0);
      
      for(i=0;i<N;i++)
	{
	x[i].pos=i;
	fscanf(fp,"%g %f",&tim,&x[i].val);
	}


	/* Mean Value of the number of Packets */
              for(i=0;i<N;i++)
		average=average+x[i].val;

	        average=average/(float)N;

        
        /* Standard Deviation */
 
             for(i=0;i<N;i++)
               sd=sd+(x[i].val-average)*(x[i].val-average);

               sd=sd/(float)N;
 

	/* Subtracting the Mean Value of the number of Packets */
              for(i=0;i<N;i++)
		x[i].val=x[i].val-average;
        

   /* Auto-correlation of N samples is calculated below */
	
         for(lag=-N+1,c=0;lag<=N-1;++lag,++c)
	  {
		ax[c].val=0;
		ax[c].pos=lag;

		for(i=0;i<N;++i)
		{
			if (i+lag>=N)
				t=0;
			else if(i+lag<0)
				t=0;
			else
				t=x[i+lag].val;

			ax[c].val=ax[c].val+(x[i].val)*(t);

		}
      
                if(lag >= 0)  	 
	 	fprintf(fautocorr,"%d %g\n",lag,ax[c].val);    
	}

        fflush(fautocorr);
	// The utilization of x is over


	/* Now the autocorrelation results are ready so got to call fftw */

	NoSamples=2*N-1;

	in=fftw_malloc(NoSamples*sizeof(fftw_complex));
	out=fftw_malloc(NoSamples*sizeof(fftw_complex));
        j=0;

	for(i=0;i<NoSamples;i++)
		{
			in[i][j]=ax[i].val;
			in[i][j+1]=0;
		}


	p=fftw_plan_dft_1d(NoSamples,in,out,FFTW_FORWARD,FFTW_ESTIMATE);

	fftw_execute(p);

	abval=(double *)malloc(N*sizeof(double));

	for(i=0;i<N;i++)
		for(j=0;j<1;j++)
		{

        	abval[i]=sqrt((out[i][j]*out[i][j])+(out[i][j+1]*out[i][j+1]));
	        freq=i/(float)NoSamples*sampling_frequency;
		fprintf(fpsd,"%g %g\n",freq,abval[i]);
		}

       fflush(fpsd);    
	
       /*To find the Cumulative power spectral density */


	cumpsd=(double *)malloc(N*sizeof(double));


	for(i=0;i<N;i++)
	{
		sum=0;
		for(j=0;j<=i;j++)
		{  
			sum=sum+abval[j];
		} 
		cumpsd[i]=sum;
	}

	// Writing the cumulative psd into another file
        // To find the 60 % quantile frequency

        flag=0;

	for(i=0;i<N;i++)
	{ 
		cumpsd[i]=cumpsd[i]/cumpsd[N-1];
                freq=i/(float)NoSamples*sampling_frequency;
                if(flag == 0)
                {
                  if((int)(10*cumpsd[i]) == 6)
                   { 
                     quantile_freq=freq;
//  fprintf(statistics,"\n\nThe 60 percent quantile frequency \n\n%g Hz",quantile_freq);
                     flag=1;
                   }
                }	
              // fprintf(fcpsd,"%g %g\n",freq,cumpsd[i]*cumpsd[N-1]);
	fprintf(fcpsd,"%g %g\n",freq,cumpsd[i]);
	}



fprintf(statistics,"Sampling frequency : %g\nNumber of Samples : %d\nMean : %g\nStandard Deviation : %g\n60 percent quantile frequency : %g",sampling_frequency,N,average,sd,quantile_freq);
	
	
//fprintf(statistics,"Sampling frequency| Number of Samples | Mean | Standard Deviation | 60 percent quantile frequency |\n %-18g | %-17d | %-4d | %-18g | %-30g | ",sampling_frequency,N,average,sd,quantile_freq);

	if(quantile_freq <= 220)
    fprintf(statistics,"\n\nMultiple source attack",quantile_freq);
    else
    fprintf(statistics,"\n\nSingle source attack",quantile_freq);
 
// Using ghostview to generate 4plot.ps ( spectral graphs )

   system("gnuplot < gopt");
 

        fflush(statistics);

	free(x);
	free(ax);
	free(abval);
	free(cumpsd);

	fftw_destroy_plan(p);
	fftw_free(in);
	fftw_free(out);

	fclose(fp);
	fclose(fpsd);
	fclose(fcpsd);
	fclose(fautocorr);
	fclose(statistics);
}
