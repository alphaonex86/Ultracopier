/* Copyright (c) 2011-2012 Xiph.Org Foundation, Mozilla Corporation
   Written by Jean-Marc Valin and Timothy B. Terriberry */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define OPUS_PI (3.14159265F)

#define OPUS_COSF(_x)        ((float)cos(_x))
#define OPUS_SINF(_x)        ((float)sin(_x))

/*static void *check_alloc(void *_ptr){
  if(_ptr==NULL){
    fprintf(stderr,"Out of memory.\n");
    exit(EXIT_FAILURE);
  }
  return _ptr;
}

static void *opus_malloc(size_t _size){
  return check_alloc(malloc(_size));
}

static void *opus_realloc(void *_ptr,size_t _size){
  return check_alloc(realloc(_ptr,_size));
}

static size_t read_pcm16(float **_samples,FILE *_fin,int _nchannels){
  unsigned char  buf[1024];
  float         *samples;
  size_t         nsamples;
  size_t         csamples;
  size_t         xi;
  size_t         nread;
  samples=NULL;
  nsamples=csamples=0;
  for(;;){
    nread=fread(buf,2*_nchannels,1024/(2*_nchannels),_fin);
    if(nread<=0)break;
    if(nsamples+nread>csamples){
      do csamples=csamples<<1|1;
      while(nsamples+nread>csamples);
      samples=(float *)opus_realloc(samples,
       _nchannels*csamples*sizeof(*samples));
    }
    for(xi=0;xi<nread;xi++){
      int ci;
      for(ci=0;ci<_nchannels;ci++){
        int s;
        s=buf[2*(xi*_nchannels+ci)+1]<<8|buf[2*(xi*_nchannels+ci)];
        s=((s&0xFFFF)^0x8000)-0x8000;
        samples[(nsamples+xi)*_nchannels+ci]=s;
      }
    }
    nsamples+=nread;
  }
  *_samples=(float *)opus_realloc(samples,
   _nchannels*nsamples*sizeof(*samples));
  return nsamples;
}

static void band_energy(float *_out,float *_ps,const int *_bands,int _nbands,
 const float *_in,int _nchannels,size_t _nframes,int _window_sz,
 int _step,int _downsample){
  float *window;
  float *x;
  float *c;
  float *s;
  size_t xi;
  int    xj;
  int    ps_sz;
  window=(float *)opus_malloc((3+_nchannels)*_window_sz*sizeof(*window));
  c=window+_window_sz;
  s=c+_window_sz;
  x=s+_window_sz;
  ps_sz=_window_sz/2;
  for(xj=0;xj<_window_sz;xj++){
    window[xj]=0.5F-0.5F*OPUS_COSF((2*OPUS_PI/(_window_sz-1))*xj);
  }
  for(xj=0;xj<_window_sz;xj++){
    c[xj]=OPUS_COSF((2*OPUS_PI/_window_sz)*xj);
  }
  for(xj=0;xj<_window_sz;xj++){
    s[xj]=OPUS_SINF((2*OPUS_PI/_window_sz)*xj);
  }
  for(xi=0;xi<_nframes;xi++){
    int ci;
    int xk;
    int bi;
    for(ci=0;ci<_nchannels;ci++){
      for(xk=0;xk<_window_sz;xk++){
        x[ci*_window_sz+xk]=window[xk]*_in[(xi*_step+xk)*_nchannels+ci];
      }
    }
    for(bi=xj=0;bi<_nbands;bi++){
      float p[2]={0};
      for(;xj<_bands[bi+1];xj++){
        for(ci=0;ci<_nchannels;ci++){
          float re;
          float im;
          int   ti;
          ti=0;
          re=im=0;
          for(xk=0;xk<_window_sz;xk++){
            re+=c[ti]*x[ci*_window_sz+xk];
            im-=s[ti]*x[ci*_window_sz+xk];
            ti+=xj;
            if(ti>=_window_sz)ti-=_window_sz;
          }
          re*=_downsample;
          im*=_downsample;
          _ps[(xi*ps_sz+xj)*_nchannels+ci]=re*re+im*im+100000;
          p[ci]+=_ps[(xi*ps_sz+xj)*_nchannels+ci];
        }
      }
      if(_out){
        _out[(xi*_nbands+bi)*_nchannels]=p[0]/(_bands[bi+1]-_bands[bi]);
        if(_nchannels==2){
          _out[(xi*_nbands+bi)*_nchannels+1]=p[1]/(_bands[bi+1]-_bands[bi]);
        }
      }
    }
  }
  free(window);
}*/

#define NBANDS (21)
#define NFREQS (240)

/*Bands on which we compute the pseudo-NMR (Bark-derived
  CELT bands).*/
/*static const int BANDS[NBANDS+1]={
  0,2,4,6,8,10,12,14,16,20,24,28,32,40,48,56,68,80,96,120,156,200
};*/

#define TEST_WIN_SIZE (480)
#define TEST_WIN_STEP (120)
