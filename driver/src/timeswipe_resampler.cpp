#include "timeswipe_resampler.hpp"
#include "upfirdn.h"
#include <boost/math/special_functions/bessel.hpp>

static unsigned getPad(unsigned samples) {
    if (samples >= 24000) return 20;
    else if (samples >= 20000) return 30;
    else if (samples >= 12000) return 40;
    else if (samples >= 9000) return 20;
    else if (samples >= 6000) return 80;
    else if (samples >= 3000) return 160;
    else if (samples >= 2000) return 300;

    return 500;
}

TimeSwipeResampler::TimeSwipeResampler(int up, int down)
    : upFactor(up)
    , downFactor(down)
{
    pad = getPad(upFactor);
}

static int getGCD ( int num1, int num2 )
{
  int tmp = 0;
  while ( num1 > 0 )
  {
    tmp = num1;
    num1 = num2 % num1;
    num2 = tmp;
  }
  return num2;
}

std::vector<Record> TimeSwipeResampler::Resample(std::vector<Record>&& records) {
    if (records.empty()) return std::vector<Record>();
    // TODO: optimization: eliminate copying
    for (const auto& r: records) {
        for (int i = 0; i < 4; i++) {
            buffers[i].push_back(r.Sensors[i]);
        }
    }
    int inputSize = buffers[0].size();
    //process input data with constant slice
    size_t slice_size = 1000;
    if (inputSize < slice_size+2*pad) return std::vector<Record>();
    inputSize = slice_size + 2*pad;
    //fprintf(stderr, "inputSize: %d\n", inputSize);
    //TODO: limit size of states
    auto ret = states.emplace(buffers[0].size(), nullptr);
    if (ret.second) {
        ret.first->second = std::make_unique<ResamplerState>(upFactor, downFactor, inputSize);
    }
    auto& state = ret.first->second;

    std::vector<float> y[4];
    for (int j = 0; j < 4; j++) {
        std::vector<float> yy;
          int gcd = getGCD ( upFactor, downFactor );
          int upf = upFactor / gcd;
          int downf = downFactor / gcd;
        upfirdn(upf, downf, &buffers[j][0], inputSize, &state->h[0], state->h.size(), yy);
        for (int i = state->delay; i < state->outputSize + state->delay; i++) {
            y[j].push_back ( yy[i] );
        }
    }
    std::vector<Record> out;
    auto sz = y[0].size();
    // remove processed slice from input data
    for (int j = 0; j < 4; j++) {
        buffers[j].erase(buffers[j].begin(), buffers[j].begin() + slice_size);
    }
    // Remove results related to pad
    int rem_pad = state->outputSize * pad / inputSize;
    for(int i=0; i < rem_pad;i++) if (sz > 0)--sz;
    for (size_t i = rem_pad; i < sz; i++) {
        out.push_back({y[0][i], y[1][i], y[2][i], y[3][i]});
    }
    return out;
}

using namespace std; // TODO
static int quotientCeil ( int num1, int num2 )
{
  if ( num1 % num2 != 0 )
    return num1 / num2 + 1;
  return num1 / num2;
}

static double sinc ( double x )
{
  if ( fabs ( x - 0.0 ) < 0.000001 )
    return 1;
  return sin ( M_PI * x ) / ( M_PI * x );
}

static void firls ( int length, vector<double> freq, 
  const vector<double>& amplitude, vector<double>& result )
{
  vector<double> weight;
  int freqSize = freq.size ();
  int weightSize = freqSize / 2;

  weight.reserve ( weightSize );
  for ( int i = 0; i < weightSize; i++ )
    weight.push_back ( 1.0 );

  int filterLength = length + 1;

  for ( int i = 0; i < freqSize; i++ )
    freq[i] /= 2.0;

  vector<double> dFreq;
  for ( int i = 1; i < freqSize; i++ )
    dFreq.push_back ( freq[i] - freq[i - 1] );

  length = ( filterLength - 1 ) / 2;
  int Nodd = filterLength % 2;
  double b0 = 0.0;
  vector<double> k;
  if ( Nodd == 0 )
  {
    for ( int i = 0; i <= length; i++)
      k.push_back ( i + 0.5 );
  }
  else
  {
    for ( int i = 0; i <= length; i++)
      k.push_back ( i );
  }

  vector<double> b;
  int kSize = k.size();
  for ( int i = 0; i < kSize; i++ )
    b.push_back( 0.0 );
  for ( int i = 0; i < freqSize; i += 2 )
  {
    double slope = ( amplitude[i + 1] - amplitude[i] ) / ( freq[i + 1] - freq[i] );
    double b1 = amplitude[i] - slope * freq[i];
    if ( Nodd == 1 )
    {
      b0 += ( b1 * ( freq[i + 1] - freq[i] ) ) + 
        slope / 2.0 * ( freq[i + 1] * freq[i + 1] - freq[i] * freq[i] ) * 
          fabs( weight[(i + 1) / 2] * weight[(i + 1) / 2] );
    }
    for ( int j = 0; j < kSize; j++ )
    {
      b[j] += ( slope / ( 4 * M_PI * M_PI ) * 
        (cos ( 2 * M_PI * k[j] * freq[i + 1] ) - cos ( 2 * M_PI * k[j] * freq[i] )) / ( k[j] * k[j] )) *
          fabs( weight[(i + 1) / 2] * weight[(i + 1) / 2] );
      b[j] += ( freq[i + 1] * ( slope * freq[i + 1] + b1 ) * sinc( 2 * k[j] * freq[i + 1] ) - 
        freq[i] * ( slope * freq[i] + b1 ) * sinc( 2 * k[j] * freq[i] ) ) *
          fabs( weight[(i + 1) / 2] * weight[(i + 1) / 2] );
    }
  }
  if ( Nodd == 1 )
    b[0] = b0;
  vector<double> a;
  double w0 = weight[0];
  for ( int i = 0; i < kSize; i++ )
    a.push_back(( w0 * w0 ) * 4 * b[i]);
  if ( Nodd == 1 )
  {
    a[0] /= 2;
    for ( int i = length; i >= 1; i-- )
      result.push_back( a[i] / 2.0 );
    result.push_back( a[0] );
    for ( int i = 1; i <= length; i++ )
      result.push_back( a[i] / 2.0 );
  }
  else
  {
    for ( int i = length; i >= 0; i-- )
      result.push_back( a[i] / 2.0 );
    for ( int i = 0; i <= length; i++ )
      result.push_back( a[i] / 2.0 );
  }
}

static void kaiser ( const int order, const double bta, vector<double>& window )
{
  double bes = fabs ( boost::math::cyl_bessel_i ( 0, bta ) );
  int odd = order % 2;
  double xind = ( order - 1 ) * ( order - 1 );
  int n = ( order + 1 ) / 2;
  vector<double> xi;
  xi.reserve ( n );
  for ( int i = 0; i < n; i++ )
  {
    double val = static_cast<double>( i ) + 0.5 * ( 1 - static_cast<double>( odd ) );
    xi.push_back ( 4 * val * val );
  }
  vector<double> w;
  w.reserve ( n );
  for ( int i = 0; i < n; i++ )
    w.push_back ( boost::math::cyl_bessel_i ( 0, bta * sqrt( 1 - xi[i] / xind ) ) / bes );
  for ( int i = n - 1; i >= odd; i-- )
    window.push_back ( fabs ( w[i] ) );
  for ( int i = 0; i < n; i++ )
    window.push_back ( fabs ( w[i] ) );
}

ResamplerState::ResamplerState(int upFactor, int downFactor, size_t inputSize) {
  const int n = 10;
  const double bta = 5.0;
  int gcd = getGCD ( upFactor, downFactor );
  upFactor /= gcd;
  downFactor /= gcd;

  outputSize =  quotientCeil ( inputSize * upFactor, downFactor );

  int maxFactor = max ( upFactor, downFactor );
  double firlsFreq = 1.0 / 2.0 / static_cast<double> ( maxFactor );
  int length = 2 * n * maxFactor + 1;
  double firlsFreqs[] = { 0.0, 2.0 * firlsFreq, 2.0 * firlsFreq, 1.0 };
  vector<double> firlsFreqsV;
  firlsFreqsV.assign ( firlsFreqs, firlsFreqs + 4 );
  double firlsAmplitude[] = { 1.0, 1.0, 0.0, 0.0 };
  vector<double> firlsAmplitudeV;
  firlsAmplitudeV.assign ( firlsAmplitude, firlsAmplitude + 4 );
  vector<double> coefficients;
  firls ( length - 1, firlsFreqsV, firlsAmplitudeV, coefficients );
  if (TimeSwipe::resample_log) {
      printf("resample: up: %d down: %d inputSize: %lu coefficients(%lu):", upFactor, downFactor, inputSize, coefficients.size());
      for (const auto& c: coefficients) printf(" %f",c);
      printf("\n");
  }
  vector<double> window;
  kaiser ( length, bta, window );
  int coefficientsSize = coefficients.size();
  for( int i = 0; i < coefficientsSize; i++ )
    coefficients[i] *= upFactor * window[i];

  int lengthHalf = ( length - 1 ) / 2;
  int nz = downFactor - lengthHalf % downFactor;
  h.clear();
  h.reserve ( coefficientsSize + nz );
  for ( int i = 0; i < nz; i++ )
    h.push_back ( 0.0 );
  for ( int i = 0; i < coefficientsSize; i++ )
    h.push_back ( coefficients[i] );
  int hSize = h.size();
  lengthHalf += nz;
  delay = lengthHalf / downFactor;
  nz = 0;
  while ( quotientCeil( ( inputSize - 1 ) * upFactor + hSize + nz, downFactor ) - delay < outputSize )
    nz++;
  for ( int i = 0; i < nz; i++ )
    h.push_back ( 0.0 );

}



//TEST
#ifdef TIMESWIPE_RESAMPLER_TEST
bool TimeSwipe::resample_log = false;
#include <fstream>
void export_resample(TimeSwipeResampler& resampler, std::vector<Record>&& records) {
    if (records.empty()) return;
    auto recs = resampler.Resample(std::move(records));
    for (const auto &r: recs) {
        std::cout << r.Sensors[0] << "\t" << r.Sensors[1] << "\t" << r.Sensors[2] << "\t" << r.Sensors[3] << std::endl;
    }
}
int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <dump> upFactor downFactor\n", argv[0]);
        return -1;
    }
    std::ifstream inFile(argv[1]);
    int upFactor = std::atoi(argv[2]);
    int downFactor = std::atoi(argv[3]);
    TimeSwipeResampler resampler(upFactor, downFactor);
    std::vector<Record> records;
    Record rec;
    unsigned counter = 0;
    //static const int chunk_size = 48000*20;
    //static const int chunk_size = 1000;
    //static const int chunk_size = 480000;
    unsigned chunk_size = rand()%100+10;
    while (inFile >> rec.Sensors[0] >> rec.Sensors[1] >> rec.Sensors[2] >> rec.Sensors[3]) {
        records.push_back(rec);
        if (++counter>=chunk_size) {
            export_resample(resampler, std::move(records));
            records.clear();
            counter = 0;
            chunk_size = rand()%100+10;
        }
    }
    export_resample(resampler, std::move(records));

    return 0;
}
#endif
