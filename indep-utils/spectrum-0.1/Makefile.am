bin_PROGRAMS = spectrum  
spectrum_SOURCES = psd.c spect.h
INCLUDES=.
EXTRA_DIST=gopt spect.h fftw-3.0.1.tar.gz  

all:   
	cc -O3 -o spectrum psd.c libfftw3.a -lm
		


