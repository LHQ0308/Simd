/*
* Simd Library (http://ermig1979.github.io/Simd).
*
* Copyright (c) 2011-2019 Yermalayeu Ihar.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/
#include "Simd/SimdConvolution.h"
#include "Simd/SimdExtract.h"
#include "Simd/SimdSynet.h"
#include "Simd/SimdAvx1.h"

namespace Simd
{
#ifdef SIMD_AVX_ENABLE    
    namespace Avx
    {
        void ConvolutionBiasAndActivation(const float * bias, size_t count, size_t size, ::SimdConvolutionActivationType activation, const float * params, ::SimdBool trans, float * dst)
        {
            size_t aligned = trans ? AlignLo(count, F) : AlignLo(size, F);
            if (activation == ::SimdConvolutionActivationIdentity)
            {
                if (bias)
                    SynetAddBias(bias, count, size, dst, trans);
            }
            else if (activation == ::SimdConvolutionActivationRelu)
            {
                if (bias)
                {
                    __m256 _0 = _mm256_set1_ps(0.0f);
                    if (trans)
                    {
                        for (size_t j = 0; j < size; ++j)
                        {
                            size_t i = 0;
                            for (; i < aligned; i += F)
                            {
                                __m256 _dst = _mm256_loadu_ps(dst + i);
                                __m256 _bias = _mm256_loadu_ps(bias + i);
                                _mm256_storeu_ps(dst + i, _mm256_max_ps(_0, _mm256_add_ps(_dst, _bias)));
                            }
                            for (; i < count; ++i)
                                dst[i] = Simd::Max(0.0f, dst[i] + bias[i]);
                            dst += count;
                        }
                    }
                    else
                    {
                        for (size_t i = 0; i < count; ++i)
                        {
                            __m256 _bias = _mm256_set1_ps(bias[i]);
                            size_t j = 0;
                            for (; j < aligned; j += F)
                            {
                                __m256 _dst = _mm256_loadu_ps(dst + j);
                                _mm256_storeu_ps(dst + j, _mm256_max_ps(_0, _mm256_add_ps(_dst, _bias)));
                            }
                            for (; j < size; ++j)
                                dst[j] = Simd::Max(0.0f, dst[j] + bias[i]);
                            dst += size;
                        }
                    }
                }
                else
                {
                    float slope = 0;
                    NeuralRelu(dst, size*count, &slope, dst);
                }
            }
            else if (activation == ::SimdConvolutionActivationLeakyRelu)
            {
                float slope = params[0];
                if (bias)
                {
                    __m256 _slope = _mm256_set1_ps(slope);
                    if (trans)
                    {
                        for (size_t j = 0; j < size; ++j)
                        {
                            size_t i = 0;
                            for (; i < aligned; i += F)
                            {
                                __m256 value = _mm256_add_ps(_mm256_loadu_ps(dst + i), _mm256_loadu_ps(bias + i));
                                _mm256_storeu_ps(dst + i, SynetPreluLayerForward(value, _slope));
                            }
                            for (; i < count; ++i)
                                dst[i] = Base::SynetPreluLayerForward(dst[i] + bias[i], slope);
                            dst += count;
                        }
                    }
                    else
                    {
                        for (size_t i = 0; i < count; ++i)
                        {
                            __m256 _bias = _mm256_set1_ps(bias[i]);
                            size_t j = 0;
                            for (; j < aligned; j += F)
                            {
                                __m256 value = _mm256_add_ps(_mm256_loadu_ps(dst + j), _bias);
                                _mm256_storeu_ps(dst + j, SynetPreluLayerForward(value, _slope));
                            }
                            for (; j < size; ++j)
                                dst[j] = Base::SynetPreluLayerForward(dst[j] + bias[i], slope);
                            dst += size;
                        }
                    }
                }
                else
                    NeuralRelu(dst, size*count, &slope, dst);
            }
            else if (activation == ::SimdConvolutionActivationRestrictRange)
            {
                float lower = params[0];
                float upper = params[1];
                if (bias)
                {
                    __m256 _lower = _mm256_set1_ps(lower);
                    __m256 _upper = _mm256_set1_ps(upper);
                    if (trans)
                    {
                        for (size_t j = 0; j < size; ++j)
                        {
                            size_t i = 0;
                            for (; i < aligned; i += F)
                            {
                                __m256 value = _mm256_add_ps(_mm256_loadu_ps(dst + i), _mm256_loadu_ps(bias + i));
                                _mm256_storeu_ps(dst + i, _mm256_min_ps(_mm256_max_ps(_lower, value), _upper));
                            }
                            for (; i < count; ++i)
                                dst[i] = Simd::RestrictRange(dst[i] + bias[i], lower, upper);
                            dst += count;
                        }
                    }
                    else
                    {
                        for (size_t i = 0; i < count; ++i)
                        {
                            __m256 _bias = _mm256_set1_ps(bias[i]);
                            size_t j = 0;
                            for (; j < aligned; j += F)
                            {
                                __m256 value = _mm256_add_ps(_mm256_loadu_ps(dst + j), _bias);
                                _mm256_storeu_ps(dst + j, _mm256_min_ps(_mm256_max_ps(_lower, value), _upper));
                            }
                            for (; j < size; ++j)
                                dst[j] = Simd::RestrictRange(dst[j] + bias[i], lower, upper);
                            dst += size;
                        }
                    }
                }
                else
                    SynetRestrictRange(dst, size*count, &lower, &upper, dst);
            }
            else if (activation == ::SimdConvolutionActivationPrelu)
            {
                if (bias)
                {
                    if (trans)
                    {
                        for (size_t j = 0; j < size; ++j)
                        {
                            size_t i = 0;
                            for (; i < aligned; i += F)
                            {
                                __m256 value = _mm256_add_ps(_mm256_loadu_ps(dst + i), _mm256_loadu_ps(bias + i));
                                _mm256_storeu_ps(dst + i, SynetPreluLayerForward(value, _mm256_loadu_ps(params + i)));
                            }
                            for (; i < count; ++i)
                                dst[i] = Base::SynetPreluLayerForward(dst[i] + bias[i], params[i]);
                            dst += count;
                        }
                    }
                    else
                    {
                        for (size_t i = 0; i < count; ++i)
                        {
                            __m256 _bias = _mm256_set1_ps(bias[i]);
                            __m256 _slope = _mm256_set1_ps(params[i]);
                            size_t j = 0;
                            for (; j < aligned; j += F)
                            {
                                __m256 value = _mm256_add_ps(_mm256_loadu_ps(dst + j), _bias);
                                _mm256_storeu_ps(dst + j, SynetPreluLayerForward(value, _slope));
                            }
                            for (; j < size; ++j)
                                dst[j] = Base::SynetPreluLayerForward(dst[j] + bias[i], params[i]);
                            dst += size;
                        }
                    }
                }
                else
                    Avx::SynetPreluLayerForward(dst, params, count, size, dst, trans);
            }
        }

        //---------------------------------------------------------------------

        ConvolutionGemmNN::ConvolutionGemmNN(const ConvParam & p)
            : Sse::ConvolutionGemmNN(p)
        {
            _gemm.Init(Avx::Gemm32fNN, "Avx", p.gemm, "Ext");
        }

        void ConvolutionGemmNN::GemmAndBias(const float * src, float * dst)
        {
            const ConvParam & p = _param;
            for (size_t g = 0; g < p.group; ++g)
            {
                if (p.srcT)
                    _gemm.Run(_M, _N, _K, &_1, src + _grS * g, _ldS, _weight + _grW * g, _ldW, &_0, dst + _grD * g, _ldD);
                else
                    _gemm.Run(_M, _N, _K, &_1, _weight + _grW * g, _ldW, src + _grS * g, _ldS, &_0, dst + _grD * g, _ldD);
            }
            Avx::ConvolutionBiasAndActivation(_bias, p.dstC, p.dstH*p.dstW, p.activation, _params, p.dstT, dst);
        }

        template<size_t size> SIMD_INLINE void Copy(const float * src, float * dst)
        {
            memcpy(dst, src, size * sizeof(float));
        }

        template<size_t size> SIMD_INLINE void Zero(float * dst)
        {
            memset(dst, 0, size * sizeof(float));
        }

        template<> SIMD_INLINE void Copy<16>(const float * src, float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_loadu_ps(src + 0 * F));
            _mm256_stream_ps(dst + 1 * F, _mm256_loadu_ps(src + 1 * F));
        }

        template<> SIMD_INLINE void Zero<16>(float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 1 * F, _mm256_setzero_ps());
        }

        template<> SIMD_INLINE void Copy<24>(const float * src, float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_loadu_ps(src + 0 * F));
            _mm256_stream_ps(dst + 1 * F, _mm256_loadu_ps(src + 1 * F));
            _mm256_stream_ps(dst + 2 * F, _mm256_loadu_ps(src + 2 * F));
        }

        template<> SIMD_INLINE void Zero<24>(float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 1 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 2 * F, _mm256_setzero_ps());
        }

        template<> SIMD_INLINE void Copy<32>(const float * src, float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_loadu_ps(src + 0 * F));
            _mm256_stream_ps(dst + 1 * F, _mm256_loadu_ps(src + 1 * F));
            _mm256_stream_ps(dst + 2 * F, _mm256_loadu_ps(src + 2 * F));
            _mm256_stream_ps(dst + 3 * F, _mm256_loadu_ps(src + 3 * F));
        }

        template<> SIMD_INLINE void Zero<32>(float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 1 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 2 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 3 * F, _mm256_setzero_ps());
        }

        template<> SIMD_INLINE void Copy<48>(const float * src, float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_loadu_ps(src + 0 * F));
            _mm256_stream_ps(dst + 1 * F, _mm256_loadu_ps(src + 1 * F));
            _mm256_stream_ps(dst + 2 * F, _mm256_loadu_ps(src + 2 * F));
            _mm256_stream_ps(dst + 3 * F, _mm256_loadu_ps(src + 3 * F));
            _mm256_stream_ps(dst + 4 * F, _mm256_loadu_ps(src + 4 * F));
            _mm256_stream_ps(dst + 5 * F, _mm256_loadu_ps(src + 5 * F));
        }

        template<> SIMD_INLINE void Zero<48>(float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 1 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 2 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 3 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 4 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 5 * F, _mm256_setzero_ps());
        }

        template<> SIMD_INLINE void Copy<64>(const float * src, float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_loadu_ps(src + 0 * F));
            _mm256_stream_ps(dst + 1 * F, _mm256_loadu_ps(src + 1 * F));
            _mm256_stream_ps(dst + 2 * F, _mm256_loadu_ps(src + 2 * F));
            _mm256_stream_ps(dst + 3 * F, _mm256_loadu_ps(src + 3 * F));
            _mm256_stream_ps(dst + 4 * F, _mm256_loadu_ps(src + 4 * F));
            _mm256_stream_ps(dst + 5 * F, _mm256_loadu_ps(src + 5 * F));
            _mm256_stream_ps(dst + 6 * F, _mm256_loadu_ps(src + 6 * F));
            _mm256_stream_ps(dst + 7 * F, _mm256_loadu_ps(src + 7 * F));
        }

        template<> SIMD_INLINE void Zero<64>(float * dst)
        {
            _mm256_stream_ps(dst + 0 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 1 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 2 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 3 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 4 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 5 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 6 * F, _mm256_setzero_ps());
            _mm256_stream_ps(dst + 7 * F, _mm256_setzero_ps());
        }

        template<size_t size> void ImgToCol(const ConvParam & p, const float * src, float * dst)
        {
            for (size_t g = 0; g < p.group; ++g)
            {
                for (size_t dy = 0; dy < p.dstH; ++dy)
                {
                    for (size_t dx = 0; dx < p.dstW; ++dx)
                    {
                        for (size_t ky = 0; ky < p.kernelY; ky++)
                        {
                            size_t sy = dy * p.strideY + ky * p.dilationY - p.padY;
                            if (sy < p.srcH)
                            {
                                for (size_t kx = 0; kx < p.kernelX; kx++)
                                {
                                    size_t sx = dx * p.strideX + kx * p.dilationX - p.padX;
                                    if (sx < p.srcW)
                                    {
                                        Copy<size>(src + (sy * p.srcW + sx)*p.srcC, dst);
                                        dst += size;
                                    }
                                    else
                                    {
                                        Zero<size>(dst);
                                        dst += size;
                                    }
                                }
                            }
                            else
                            {
                                for (size_t kx = 0; kx < p.kernelX; kx++)
                                    Zero<size>(dst), dst += size;
                            }
                        }
                    }
                }
                src += size;
            }
        }

        void ConvolutionGemmNN::ImgToRow(const float * src, float * dst)
        {
            SIMD_PERF_BEG(_param.Info());

            const ConvParam & p = _param;
            assert(p.srcT == ::SimdTrue);
            size_t size = p.srcC / p.group;
            if (size*p.dstH*p.dstW*p.kernelY*p.kernelX >= 1024 * 512 && Aligned(dst))
            {
                if (size == 16)
                {
                    Avx::ImgToCol<16>(p, src, dst);
                    return;
                }
                if (size == 24)
                {
                    Avx::ImgToCol<24>(p, src, dst);
                    return;
                }
                if (size == 32)
                {
                    Avx::ImgToCol<32>(p, src, dst);
                    return;
                }
                if (size == 48)
                {
                    Avx::ImgToCol<48>(p, src, dst);
                    return;
                }
                if (size == 64)
                {
                    Avx::ImgToCol<64>(p, src, dst);
                    return;
                }
            }
            for (size_t g = 0; g < p.group; ++g)
            {
                for (size_t dy = 0; dy < p.dstH; ++dy)
                {
                    for (size_t dx = 0; dx < p.dstW; ++dx)
                    {
                        for (size_t ky = 0; ky < p.kernelY; ky++)
                        {
                            size_t sy = dy * p.strideY + ky * p.dilationY - p.padY;
                            if (sy < p.srcH)
                            {
                                for (size_t kx = 0; kx < p.kernelX; kx++)
                                {
                                    size_t sx = dx * p.strideX + kx * p.dilationX - p.padX;
                                    if (sx < p.srcW)
                                    {
                                        memcpy(dst, src + (sy * p.srcW + sx)*p.srcC, size * sizeof(float));
                                        dst += size;
                                    }
                                    else
                                    {
                                        memset(dst, 0, size * sizeof(float));
                                        dst += size;
                                    }
                                }
                            }
                            else
                            {
                                memset(dst, 0, p.kernelX * size * sizeof(float));
                                dst += p.kernelX * size;
                            }
                        }
                    }
                }
                src += size;
            }
        }

        //---------------------------------------------------------------------

        ConvolutionGemmNT::ConvolutionGemmNT(const ConvParam & p)
            : Sse3::ConvolutionGemmNT(p)
        {
        }

        void ConvolutionGemmNT::GemmAndBias(const float * src, float * dst)
        {
            const ConvParam & p = _param;
            for (size_t g = 0; g < p.group; ++g)
                Avx::Gemm32fNT(_M, _N, _K, &_1, _weight + _weightStep * g, _K, src + _srcStep * g, _K, &_0, dst + _dstStep * g, _N);
            Avx::ConvolutionBiasAndActivation(_bias, p.dstC, p.dstH*p.dstW, p.activation, _params, ::SimdFalse, dst);
        }

        //---------------------------------------------------------------------

        ConvolutionWinograd2x3p::ConvolutionWinograd2x3p(const ConvParam & p)
            : Sse::ConvolutionWinograd2x3p(p)
        {
            _setFilter = Avx::Winograd2x3SetFilter;
            _gemm.Init(Avx::Gemm32fNN, "Avx", p.gemm, "Ext");
        }

        void ConvolutionWinograd2x3p::Forward(const float * src, float * buf, float * dst)
        {
            const ConvParam & p = _param;
            float * bufS = Buffer(buf);
            float * bufD = bufS + _strideS * _count;
            Avx::Winograd2x3SetInput(src, p.srcC, p.srcH, p.srcW, buf, _pad, p.srcT);
            for (size_t i = 0; i < _count; ++i)
            {
                if (p.srcT)
                    _gemm.Run(_M, _N, _K, &_1, bufS + i * _strideS, _K, _weight.data + i * _strideW, _N, &_0, bufD + i * _strideD, _N);
                else
                    _gemm.Run(_M, _N, _K, &_1, _weight.data + i * _strideW, _K, bufS + i * _strideS, _N, &_0, bufD + i * _strideD, _N);
            }
            Avx::Winograd2x3SetOutput(bufD, dst, p.dstC, p.dstH, p.dstW, p.dstT);
            Avx::ConvolutionBiasAndActivation(_bias, p.dstC, p.dstH*p.dstW, p.activation, _params, p.dstT, dst);
        }

        //---------------------------------------------------------------------

        ConvolutionDirectChw::ConvolutionDirectChw(const ConvParam & p)
            : Sse::ConvolutionDirectChw(p)
        {
            _convolutionBiasActivation = SetConvolutionBiasActivation();
        }

        template <size_t size> SIMD_INLINE void LoadWeight(const float * src, __m256 * dst)
        {
            for (size_t i = 0; i < size; ++i)
                dst[i] = _mm256_set1_ps(src[i]);
        }

        template<int kernel, int stride> struct Kernel
        {
            static __m256 Convolution(const float * src, size_t step, const __m256  * weight);
        };

        template<> struct Kernel<1, 1>
        {
            static SIMD_INLINE __m256 Convolution(const float * src, size_t step, const __m256  * weight)
            {
                return _mm256_mul_ps(_mm256_loadu_ps(src), weight[0]);
            }
        };

        template<> struct Kernel<2, 1>
        {
            static SIMD_INLINE __m256 RowConv(const float * src, const __m256  * weight)
            {
                return _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(src + 0), weight[0]), _mm256_mul_ps(_mm256_loadu_ps(src + 1), weight[1]));
            }

            static SIMD_INLINE __m256 Convolution(const float * src, size_t step, const __m256  * weight)
            {
                return _mm256_add_ps(RowConv(src, weight), RowConv(src + step, weight + 2));
            }
        };

        template<> struct Kernel<3, 1>
        {
            static SIMD_INLINE __m256 RowConv(const float * src, const __m256  * weight)
            {
                return _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(src), weight[0]),
                    _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(src + 1), weight[1]),
                        _mm256_mul_ps(_mm256_loadu_ps(src + 2), weight[2])));
            }

            static SIMD_INLINE __m256 Convolution(const float * src, size_t step, const __m256  * weight)
            {
                return _mm256_add_ps(RowConv(src, weight),
                    _mm256_add_ps(RowConv(src + step, weight + 3),
                        RowConv(src + 2 * step, weight + 6)));
            }
        };

        template<::SimdConvolutionActivationType type> SIMD_INLINE __m256 Activate(__m256 value, const __m256 * params);

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationIdentity>(__m256 value, const __m256 * params)
        {
            return value;
        }

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationRelu>(__m256 value, const __m256 * params)
        {
            return _mm256_max_ps(_mm256_setzero_ps(), value);
        }

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationLeakyRelu>(__m256 value, const __m256 * params)
        {
            return _mm256_add_ps(_mm256_max_ps(_mm256_setzero_ps(), value), _mm256_mul_ps(params[0], _mm256_min_ps(_mm256_setzero_ps(), value)));
        }

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationRestrictRange>(__m256 value, const __m256 * params)
        {
            return _mm256_min_ps(_mm256_max_ps(params[0], value), params[1]);
        }

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationPrelu>(__m256 value, const __m256 * params)
        {
            return _mm256_add_ps(_mm256_max_ps(_mm256_setzero_ps(), value), _mm256_mul_ps(params[0], _mm256_min_ps(_mm256_setzero_ps(), value)));
        }

        template<int kernel, int stride, ::SimdConvolutionActivationType type> 
        void ConvolutionBiasActivation(const float * src, size_t srcC, size_t srcH, size_t srcW, const float * weight, 
            const float * bias, const float * params, float * dst, size_t dstC, size_t dstH, size_t dstW)
        {
            __m256 _weight[kernel*kernel];
            __m256 _params[2];
            _params[0] = _mm256_set1_ps(params[0]);
            if (type == ::SimdConvolutionActivationRestrictRange)
                _params[1] = _mm256_set1_ps(params[1]);
            size_t dstWF = Simd::AlignLo(dstW, F);
            __m256 tail = RightNotZero(dstW - dstWF);
            for (size_t dc = 0; dc < dstC; ++dc)
            {
                if (type == ::SimdConvolutionActivationPrelu)
                    _params[0] = _mm256_set1_ps(params[dc]);
                if (srcC == 1)
                {
                    const float * ps = src;
                    float * pd = dst;
                    LoadWeight<kernel*kernel>(weight, _weight);
                    __m256 _bias = bias ? _mm256_set1_ps(bias[dc]) : _mm256_setzero_ps();
                    for (size_t y = 0; y < dstH; ++y)
                    {
                        for (size_t x = 0; x < dstWF; x += F)
                        {
                            __m256 conv = Kernel<kernel, stride>::Convolution(ps + x * stride, srcW, _weight);
                            _mm256_storeu_ps(pd + x, Activate<type>(_mm256_add_ps(_bias, conv), _params));
                        }
                        if (dstWF < dstW)
                        {
                            size_t x = dstW - F;
                            __m256 _dst = _mm256_loadu_ps(pd + x);
                            __m256 conv = Kernel<kernel, stride>::Convolution(ps + x * stride, srcW, _weight);
                            _mm256_storeu_ps(pd + x, _mm256_blendv_ps(_dst, Activate<type>(_mm256_add_ps(_bias, conv), _params), tail));
                        }
                        ps += srcW * stride;
                        pd += dstW;
                    }
                    weight += kernel * kernel;
                }
                else
                {
                    size_t sc = 0;
                    for (; sc < 1; ++sc)
                    {
                        const float * ps = src;
                        float * pd = dst;
                        LoadWeight<kernel*kernel>(weight, _weight);
                        __m256 _bias = bias ? _mm256_set1_ps(bias[dc]) : _mm256_setzero_ps();
                        for (size_t y = 0; y < dstH; ++y)
                        {
                            for (size_t x = 0; x < dstWF; x += F)
                            {
                                __m256 conv = Kernel<kernel, stride>::Convolution(ps + x * stride, srcW, _weight);
                                _mm256_storeu_ps(pd + x, _mm256_add_ps(_bias, conv));
                            }
                            if (dstWF < dstW)
                            {
                                size_t x = dstW - F;
                                __m256 _dst = _mm256_loadu_ps(pd + x);
                                __m256 conv = Kernel<kernel, stride>::Convolution(ps + x * stride, srcW, _weight);
                                _mm256_storeu_ps(pd + x, _mm256_blendv_ps(_dst, _mm256_add_ps(_bias, conv), tail));
                            }
                            ps += srcW * stride;
                            pd += dstW;
                        }
                        weight += kernel * kernel;
                    }
                    for (; sc < srcC - 1; ++sc)
                    {
                        const float * ps = src + sc * srcW * srcH;
                        float * pd = dst;
                        LoadWeight<kernel*kernel>(weight, _weight);
                        for (size_t y = 0; y < dstH; ++y)
                        {
                            for (size_t x = 0; x < dstWF; x += F)
                            {
                                __m256 _dst = _mm256_loadu_ps(pd + x);
                                __m256 conv = Kernel<kernel, stride>::Convolution(ps + x * stride, srcW, _weight);
                                _mm256_storeu_ps(pd + x, _mm256_add_ps(_dst, conv));
                            }
                            if (dstWF < dstW)
                            {
                                size_t x = dstW - F;
                                __m256 _dst = _mm256_loadu_ps(pd + x);
                                __m256 conv = Kernel<kernel, stride>::Convolution(ps + x * stride, srcW, _weight);
                                _mm256_storeu_ps(pd + x, _mm256_add_ps(_dst, _mm256_and_ps(conv, tail)));
                            }
                            ps += srcW * stride;
                            pd += dstW;
                        }
                        weight += kernel * kernel;
                    }
                    for (; sc < srcC; ++sc)
                    {
                        const float * ps = src + sc * srcW * srcH;
                        float * pd = dst;
                        LoadWeight<kernel*kernel>(weight, _weight);
                        for (size_t y = 0; y < dstH; ++y)
                        {
                            for (size_t x = 0; x < dstWF; x += F)
                            {
                                __m256 _dst = _mm256_loadu_ps(pd + x);
                                __m256 conv = Kernel<kernel, stride>::Convolution(ps + x * stride, srcW, _weight);
                                _mm256_storeu_ps(pd + x, Activate<type>(_mm256_add_ps(_dst, conv), _params));
                            }
                            if (dstWF < dstW)
                            {
                                size_t x = dstW - F;
                                __m256 _dst = _mm256_loadu_ps(pd + x);
                                __m256 conv = Kernel<kernel, stride>::Convolution(ps + x * stride, srcW, _weight);
                                _mm256_storeu_ps(pd + x, _mm256_blendv_ps(_dst, Activate<type>(_mm256_add_ps(_dst, conv), _params), tail));
                            }
                            ps += srcW * stride;
                            pd += dstW;
                        }
                        weight += kernel * kernel;
                    }
                }
                dst += dstH * dstW;
            }
        }

        template <int kernel, int stride> ConvolutionDirectChw::ConvolutionBiasActivationPtr SetConvolutionBiasActivation(::SimdConvolutionActivationType type)
        {
            switch (type)
            {
            case ::SimdConvolutionActivationIdentity: return ConvolutionBiasActivation<kernel, stride, ::SimdConvolutionActivationIdentity>;
            case ::SimdConvolutionActivationRelu: return ConvolutionBiasActivation<kernel, stride, ::SimdConvolutionActivationRelu>;
            case ::SimdConvolutionActivationLeakyRelu: return ConvolutionBiasActivation<kernel, stride, ::SimdConvolutionActivationLeakyRelu>;
            case ::SimdConvolutionActivationRestrictRange: return ConvolutionBiasActivation<kernel, stride, ::SimdConvolutionActivationRestrictRange>;
            case ::SimdConvolutionActivationPrelu: return ConvolutionBiasActivation<kernel, stride, ::SimdConvolutionActivationPrelu>;
            default:
                assert(0);
                return NULL;
            }
        }

        ConvolutionDirectChw::ConvolutionBiasActivationPtr ConvolutionDirectChw::SetConvolutionBiasActivation()
        {
            const ConvParam & p = _param;
            if (p.dstW < F)
                return Sse::ConvolutionDirectChw::SetConvolutionBiasActivation();
            switch (p.strideX)
            {
            case 1:
                if (p.kernelX == 1)
                    return Avx::SetConvolutionBiasActivation<1, 1>(p.activation);
                if (p.kernelX == 2)
                    return Avx::SetConvolutionBiasActivation<2, 1>(p.activation);
                if (p.kernelX == 3)
                    return Avx::SetConvolutionBiasActivation<3, 1>(p.activation);
                break;
            }
            return Sse::ConvolutionDirectChw::SetConvolutionBiasActivation();
        }

        //---------------------------------------------------------------------

        ConvolutionDirectHwc::ConvolutionDirectHwc(const ConvParam & p)
            : Sse::ConvolutionDirectHwc(p)
        {
            _convolutionBiasActivation = SetConvolutionBiasActivation();
        }

        bool ConvolutionDirectHwc::Preferable(const ConvParam & p)
        {
            if (!p.IsDilation(1) || !p.IsHwc())
                return false;
            if (p.group == 1)
            {
                if (p.kernelY > p.srcH || p.kernelX > p.srcW)
                    return false;
                return p.srcC <= 16;
            }
            else if (p.IsDepthwise())
            {
                return true;
            }
            return false;
        }

        template<::SimdConvolutionActivationType type> SIMD_INLINE __m256 Activate(__m256 value, const float * params, size_t offset);

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationIdentity>(__m256 value, const float * params, size_t offset)
        {
            return value;
        }

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationRelu>(__m256 value, const float * params, size_t offset)
        {
            return _mm256_max_ps(_mm256_setzero_ps(), value);
        }

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationLeakyRelu>(__m256 value, const float * params, size_t offset)
        {
            return _mm256_add_ps(_mm256_max_ps(_mm256_setzero_ps(), value), _mm256_mul_ps(_mm256_set1_ps(params[0]), _mm256_min_ps(_mm256_setzero_ps(), value)));
        }

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationRestrictRange>(__m256 value, const float * params, size_t offset)
        {
            return _mm256_min_ps(_mm256_max_ps(_mm256_set1_ps(params[0]), value), _mm256_set1_ps(params[1]));
        }

        template<> SIMD_INLINE __m256 Activate<::SimdConvolutionActivationPrelu>(__m256 value, const float * params, size_t offset)
        {
            return _mm256_add_ps(_mm256_max_ps(_mm256_setzero_ps(), value), _mm256_mul_ps(_mm256_loadu_ps(params + offset), _mm256_min_ps(_mm256_setzero_ps(), value)));
        }

        SIMD_INLINE void KernelHwcDefaultEdge(const float * src, const ConvParam & p, size_t kH, size_t kW, const float * weight, __m256 & sum)
        {
            size_t size = kW * p.srcC, tail = (p.kernelX - kW)*p.srcC*p.dstC, dstC = p.dstC, stride = p.srcW * p.srcC;
            for (size_t ky = 0; ky < kH; ++ky)
            {
                for (size_t i = 0; i < size; ++i, weight += dstC)
                    sum = _mm256_add_ps(_mm256_mul_ps(_mm256_set1_ps(src[i]), _mm256_loadu_ps(weight)), sum);
                weight += tail;
                src += stride;
            }
        }

        template<::SimdConvolutionActivationType type>
        SIMD_INLINE void KernelHwcDefaultEdge(const float * src, const ConvParam & p, size_t kH, size_t kW, const float * weight, const float * bias, const float * params, float * dst)
        {
            size_t dstC = p.dstC;
            size_t dstCF = AlignLo(dstC, F);
            size_t dc = 0;
            for (; dc < dstCF; dc += F)
            {
                __m256 conv = bias ? _mm256_loadu_ps(bias + dc) : _mm256_setzero_ps();
                KernelHwcDefaultEdge(src, p, kH, kW, weight + dc, conv);
                _mm256_storeu_ps(dst + dc, Activate<type>(conv, params, dc));
            }
            if (dc < dstC)
            {
                dc = dstC - F;
                __m256 conv = bias ? _mm256_loadu_ps(bias + dc) : _mm256_setzero_ps();
                KernelHwcDefaultEdge(src, p, kH, kW, weight + dc, conv);
                _mm256_storeu_ps(dst + dc, Activate<type>(conv, params, dc));
            }
        }

        SIMD_INLINE void KernelHwcDefaultBody2x2(const float * src, const ConvParam & p, const float * weight, __m256 sums[2][2])
        {
            size_t size = p.kernelX * p.srcC, dstC = p.dstC, stride = p.srcW * p.srcC, step = p.srcC * p.strideX;
            const float * src0 = src + 0 * step;
            const float * src1 = src + 1 * step;
            __m256 w0, w1, s0;
            for (size_t ky = 0; ky < p.kernelY; ++ky)
            {
                size_t offset = ky * stride;
                for (size_t end = offset + size; offset < end; ++offset)
                {
                    w0 = _mm256_loadu_ps(weight + 0 * F);
                    w1 = _mm256_loadu_ps(weight + 1 * F);
                    s0 = _mm256_set1_ps(src0[offset]);
                    sums[0][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[0][0]);
                    sums[0][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[0][1]);
                    s0 = _mm256_set1_ps(src1[offset]);
                    sums[1][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[1][0]);
                    sums[1][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[1][1]);
                    weight += dstC;
                }
            }
        }

        SIMD_INLINE void KernelHwcDefaultBody2x1(const float * src, const ConvParam & p, const float * weight, __m256 sums[2][1])
        {
            size_t size = p.kernelX * p.srcC, dstC = p.dstC, stride = p.srcW * p.srcC, step = p.srcC * p.strideX;
            const float * src0 = src + 0 * step;
            const float * src1 = src + 1 * step;
            __m256 w0, s0;
            for (size_t ky = 0; ky < p.kernelY; ++ky)
            {
                size_t offset = ky * stride;
                for (size_t end = offset + size; offset < end; ++offset)
                {
                    w0 = _mm256_loadu_ps(weight + 0 * F);
                    s0 = _mm256_set1_ps(src0[offset]);
                    sums[0][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[0][0]);
                    s0 = _mm256_set1_ps(src1[offset]);
                    sums[1][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[1][0]);
                    weight += dstC;
                }
            }
        }

        template<::SimdConvolutionActivationType type>
        SIMD_INLINE void KernelHwcDefaultBody2(const float * src, const ConvParam & p, const float * weight, const float * bias, const float * params, float * dst)
        {
            size_t dstC = p.dstC;
            size_t dstCF1 = AlignLo(dstC, 1 * F);
            size_t dstCF2 = AlignLo(dstC, 2 * F);
            size_t dc = 0;
            for (; dc < dstCF2; dc += 2 * F)
            {
                __m256 sums[2][2];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc + 0 * F) : _mm256_setzero_ps();
                __m256 bias1 = bias ? _mm256_loadu_ps(bias + dc + 1 * F) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[0][1] = bias1;
                sums[1][0] = bias0;
                sums[1][1] = bias1;
                KernelHwcDefaultBody2x2(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC + 0 * F, Activate<type>(sums[0][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 0 * dstC + 1 * F, Activate<type>(sums[0][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 1 * dstC + 0 * F, Activate<type>(sums[1][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 1 * dstC + 1 * F, Activate<type>(sums[1][1], params, dc + 1 * F));
            }
            for (; dc < dstCF1; dc += 1 * F)
            {
                __m256 sums[2][1];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[1][0] = bias0;
                KernelHwcDefaultBody2x1(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC, Activate<type>(sums[0][0], params, dc));
                _mm256_storeu_ps(dst + dc + 1 * dstC, Activate<type>(sums[1][0], params, dc));
            }
            if (dc < dstC)
            {
                dc = dstC - F;
                __m256 sums[2][1];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[1][0] = bias0;
                KernelHwcDefaultBody2x1(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC, Activate<type>(sums[0][0], params, dc));
                _mm256_storeu_ps(dst + dc + 1 * dstC, Activate<type>(sums[1][0], params, dc));
            }
        }

        SIMD_INLINE void KernelHwcDefaultBody6x2(const float * src, const ConvParam & p, const float * weight, __m256 sums[6][2])
        {
            size_t size = p.kernelX * p.srcC, dstC = p.dstC, stride = p.srcW * p.srcC, step = p.srcC * p.strideX;
            const float * src0 = src + 0 * step;
            const float * src1 = src + 1 * step;
            const float * src2 = src + 2 * step;
            const float * src3 = src + 3 * step;
            const float * src4 = src + 4 * step;
            const float * src5 = src + 5 * step;
            __m256 w0, w1, s0;
            for (size_t ky = 0; ky < p.kernelY; ++ky)
            {
                size_t offset = ky * stride;
                for (size_t end = offset + size; offset < end; ++offset)
                {
                    w0 = _mm256_loadu_ps(weight + 0 * F);
                    w1 = _mm256_loadu_ps(weight + 1 * F);
                    s0 = _mm256_set1_ps(src0[offset]);
                    sums[0][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[0][0]);
                    sums[0][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[0][1]);
                    s0 = _mm256_set1_ps(src1[offset]);
                    sums[1][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[1][0]);
                    sums[1][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[1][1]);
                    s0 = _mm256_set1_ps(src2[offset]);
                    sums[2][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[2][0]);
                    sums[2][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[2][1]);
                    s0 = _mm256_set1_ps(src3[offset]);
                    sums[3][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[3][0]);
                    sums[3][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[3][1]);
                    s0 = _mm256_set1_ps(src4[offset]);
                    sums[4][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[4][0]);
                    sums[4][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[4][1]);
                    s0 = _mm256_set1_ps(src5[offset]);
                    sums[5][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[5][0]);
                    sums[5][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[5][1]);
                    weight += dstC;
                }
            }
        }

        SIMD_INLINE void KernelHwcDefaultBody6x1(const float * src, const ConvParam & p, const float * weight, __m256 sums[6][1])
        {
            size_t size = p.kernelX * p.srcC, dstC = p.dstC, stride = p.srcW * p.srcC, step = p.srcC * p.strideX;
            const float * src0 = src + 0 * step;
            const float * src1 = src + 1 * step;
            const float * src2 = src + 2 * step;
            const float * src3 = src + 3 * step;
            const float * src4 = src + 4 * step;
            const float * src5 = src + 5 * step;
            __m256 w0, s0;
            for (size_t ky = 0; ky < p.kernelY; ++ky)
            {
                size_t offset = ky * stride;
                for (size_t end = offset + size; offset < end; ++offset)
                {
                    w0 = _mm256_loadu_ps(weight + 0 * F);
                    s0 = _mm256_set1_ps(src0[offset]);
                    sums[0][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[0][0]);
                    s0 = _mm256_set1_ps(src1[offset]);
                    sums[1][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[1][0]);
                    s0 = _mm256_set1_ps(src2[offset]);
                    sums[2][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[2][0]);
                    s0 = _mm256_set1_ps(src3[offset]);
                    sums[3][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[3][0]);
                    s0 = _mm256_set1_ps(src4[offset]);
                    sums[4][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[4][0]);
                    s0 = _mm256_set1_ps(src5[offset]);
                    sums[5][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[5][0]);
                    weight += dstC;
                }
            }
        }

        template<::SimdConvolutionActivationType type>
        SIMD_INLINE void KernelHwcDefaultBody6(const float * src, const ConvParam & p, const float * weight, const float * bias, const float * params, float * dst)
        {
            size_t dstC = p.dstC;
            size_t dstCF1 = AlignLo(dstC, 1 * F);
            size_t dstCF2 = AlignLo(dstC, 2 * F);
            size_t dc = 0;
            for (; dc < dstCF2; dc += 2 * F)
            {
                __m256 sums[6][2];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc + 0 * F) : _mm256_setzero_ps();
                __m256 bias1 = bias ? _mm256_loadu_ps(bias + dc + 1 * F) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[0][1] = bias1;
                sums[1][0] = bias0;
                sums[1][1] = bias1;
                sums[2][0] = bias0;
                sums[2][1] = bias1;
                sums[3][0] = bias0;
                sums[3][1] = bias1;
                sums[4][0] = bias0;
                sums[4][1] = bias1;
                sums[5][0] = bias0;
                sums[5][1] = bias1;
                KernelHwcDefaultBody6x2(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC + 0 * F, Activate<type>(sums[0][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 0 * dstC + 1 * F, Activate<type>(sums[0][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 1 * dstC + 0 * F, Activate<type>(sums[1][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 1 * dstC + 1 * F, Activate<type>(sums[1][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 2 * dstC + 0 * F, Activate<type>(sums[2][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 2 * dstC + 1 * F, Activate<type>(sums[2][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 3 * dstC + 0 * F, Activate<type>(sums[3][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 3 * dstC + 1 * F, Activate<type>(sums[3][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 4 * dstC + 0 * F, Activate<type>(sums[4][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 4 * dstC + 1 * F, Activate<type>(sums[4][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 5 * dstC + 0 * F, Activate<type>(sums[5][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 5 * dstC + 1 * F, Activate<type>(sums[5][1], params, dc + 1 * F));
            }
            for (; dc < dstCF1; dc += 1 * F)
            {
                __m256 sums[6][1];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[1][0] = bias0;
                sums[2][0] = bias0;
                sums[3][0] = bias0;
                sums[4][0] = bias0;
                sums[5][0] = bias0;
                KernelHwcDefaultBody6x1(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC, Activate<type>(sums[0][0], params, dc));
                _mm256_storeu_ps(dst + dc + 1 * dstC, Activate<type>(sums[1][0], params, dc));
                _mm256_storeu_ps(dst + dc + 2 * dstC, Activate<type>(sums[2][0], params, dc));
                _mm256_storeu_ps(dst + dc + 3 * dstC, Activate<type>(sums[3][0], params, dc));
                _mm256_storeu_ps(dst + dc + 4 * dstC, Activate<type>(sums[4][0], params, dc));
                _mm256_storeu_ps(dst + dc + 5 * dstC, Activate<type>(sums[5][0], params, dc));
            }
            if (dc < dstC)
            {
                dc = dstC - F;
                __m256 sums[6][1];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[1][0] = bias0;
                sums[2][0] = bias0;
                sums[3][0] = bias0;
                sums[4][0] = bias0;
                sums[5][0] = bias0;
                KernelHwcDefaultBody6x1(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC, Activate<type>(sums[0][0], params, dc));
                _mm256_storeu_ps(dst + dc + 1 * dstC, Activate<type>(sums[1][0], params, dc));
                _mm256_storeu_ps(dst + dc + 2 * dstC, Activate<type>(sums[2][0], params, dc));
                _mm256_storeu_ps(dst + dc + 3 * dstC, Activate<type>(sums[3][0], params, dc));
                _mm256_storeu_ps(dst + dc + 4 * dstC, Activate<type>(sums[4][0], params, dc));
                _mm256_storeu_ps(dst + dc + 5 * dstC, Activate<type>(sums[5][0], params, dc));
            }
        }

        SIMD_INLINE void KernelHwcDefaultBody4x3(const float * src, const ConvParam & p, const float * weight, __m256 sums[4][3])
        {
            size_t size = p.kernelX * p.srcC, dstC = p.dstC, stride = p.srcW * p.srcC, step = p.srcC * p.strideX;
            const float * src0 = src + 0 * step;
            const float * src1 = src + 1 * step;
            const float * src2 = src + 2 * step;
            const float * src3 = src + 3 * step;
            __m256 w0, w1, w2, s0;
            for (size_t ky = 0; ky < p.kernelY; ++ky)
            {
                size_t offset = ky * stride;
                for (size_t end = offset + size; offset < end; ++offset)
                {
                    w0 = _mm256_loadu_ps(weight + 0 * F);
                    w1 = _mm256_loadu_ps(weight + 1 * F);
                    w2 = _mm256_loadu_ps(weight + 2 * F);
                    s0 = _mm256_set1_ps(src0[offset]);
                    sums[0][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[0][0]);
                    sums[0][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[0][1]);
                    sums[0][2] = _mm256_add_ps(_mm256_mul_ps(s0, w2), sums[0][2]);
                    s0 = _mm256_set1_ps(src1[offset]);
                    sums[1][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[1][0]);
                    sums[1][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[1][1]);
                    sums[1][2] = _mm256_add_ps(_mm256_mul_ps(s0, w2), sums[1][2]);
                    s0 = _mm256_set1_ps(src2[offset]);
                    sums[2][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[2][0]);
                    sums[2][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[2][1]);
                    sums[2][2] = _mm256_add_ps(_mm256_mul_ps(s0, w2), sums[2][2]);
                    s0 = _mm256_set1_ps(src3[offset]);
                    sums[3][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[3][0]);
                    sums[3][1] = _mm256_add_ps(_mm256_mul_ps(s0, w1), sums[3][1]);
                    sums[3][2] = _mm256_add_ps(_mm256_mul_ps(s0, w2), sums[3][2]);
                    weight += dstC;
                }
            }
        }

        SIMD_INLINE void KernelHwcDefaultBody4x1(const float * src, const ConvParam & p, const float * weight, __m256 sums[4][1])
        {
            size_t size = p.kernelX * p.srcC, dstC = p.dstC, stride = p.srcW * p.srcC, step = p.srcC * p.strideX;
            const float * src0 = src + 0 * step;
            const float * src1 = src + 1 * step;
            const float * src2 = src + 2 * step;
            const float * src3 = src + 3 * step;
            __m256 w0, s0;
            for (size_t ky = 0; ky < p.kernelY; ++ky)
            {
                size_t offset = ky * stride;
                for (size_t end = offset + size; offset < end; ++offset)
                {
                    w0 = _mm256_loadu_ps(weight + 0 * F);
                    s0 = _mm256_set1_ps(src0[offset]);
                    sums[0][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[0][0]);
                    s0 = _mm256_set1_ps(src1[offset]);
                    sums[1][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[1][0]);
                    s0 = _mm256_set1_ps(src2[offset]);
                    sums[2][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[2][0]);
                    s0 = _mm256_set1_ps(src3[offset]);
                    sums[3][0] = _mm256_add_ps(_mm256_mul_ps(s0, w0), sums[3][0]);
                    weight += dstC;
                }
            }
        }

        template<::SimdConvolutionActivationType type>
        SIMD_INLINE void KernelHwcDefaultBody4(const float * src, const ConvParam & p, const float * weight, const float * bias, const float * params, float * dst)
        {
            size_t dstC = p.dstC;
            size_t dstCF1 = AlignLo(dstC, 1 * F);
            size_t dstCF3 = AlignLoAny(dstC, 3 * F);
            size_t dc = 0;
            for (; dc < dstCF3; dc += 3 * F)
            {
                __m256 sums[4][3];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc + 0 * F) : _mm256_setzero_ps();
                __m256 bias1 = bias ? _mm256_loadu_ps(bias + dc + 1 * F) : _mm256_setzero_ps();
                __m256 bias2 = bias ? _mm256_loadu_ps(bias + dc + 2 * F) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[0][1] = bias1;
                sums[0][2] = bias2;
                sums[1][0] = bias0;
                sums[1][1] = bias1;
                sums[1][2] = bias2;
                sums[2][0] = bias0;
                sums[2][1] = bias1;
                sums[2][2] = bias2;
                sums[3][0] = bias0;
                sums[3][1] = bias1;
                sums[3][2] = bias2;
                KernelHwcDefaultBody4x3(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC + 0 * F, Activate<type>(sums[0][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 0 * dstC + 1 * F, Activate<type>(sums[0][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 0 * dstC + 2 * F, Activate<type>(sums[0][2], params, dc + 2 * F));
                _mm256_storeu_ps(dst + dc + 1 * dstC + 0 * F, Activate<type>(sums[1][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 1 * dstC + 1 * F, Activate<type>(sums[1][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 1 * dstC + 2 * F, Activate<type>(sums[1][2], params, dc + 2 * F));
                _mm256_storeu_ps(dst + dc + 2 * dstC + 0 * F, Activate<type>(sums[2][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 2 * dstC + 1 * F, Activate<type>(sums[2][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 2 * dstC + 2 * F, Activate<type>(sums[2][2], params, dc + 2 * F));
                _mm256_storeu_ps(dst + dc + 3 * dstC + 0 * F, Activate<type>(sums[3][0], params, dc + 0 * F));
                _mm256_storeu_ps(dst + dc + 3 * dstC + 1 * F, Activate<type>(sums[3][1], params, dc + 1 * F));
                _mm256_storeu_ps(dst + dc + 3 * dstC + 2 * F, Activate<type>(sums[3][2], params, dc + 2 * F));
            }
            for (; dc < dstCF1; dc += 1 * F)
            {
                __m256 sums[4][1];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[1][0] = bias0;
                sums[2][0] = bias0;
                sums[3][0] = bias0;
                KernelHwcDefaultBody4x1(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC, Activate<type>(sums[0][0], params, dc));
                _mm256_storeu_ps(dst + dc + 1 * dstC, Activate<type>(sums[1][0], params, dc));
                _mm256_storeu_ps(dst + dc + 2 * dstC, Activate<type>(sums[2][0], params, dc));
                _mm256_storeu_ps(dst + dc + 3 * dstC, Activate<type>(sums[3][0], params, dc));
            }
            if (dc < dstC)
            {
                dc = dstC - F;
                __m256 sums[4][1];
                __m256 bias0 = bias ? _mm256_loadu_ps(bias + dc) : _mm256_setzero_ps();
                sums[0][0] = bias0;
                sums[1][0] = bias0;
                sums[2][0] = bias0;
                sums[3][0] = bias0;
                KernelHwcDefaultBody4x1(src, p, weight + dc, sums);
                _mm256_storeu_ps(dst + dc + 0 * dstC, Activate<type>(sums[0][0], params, dc));
                _mm256_storeu_ps(dst + dc + 1 * dstC, Activate<type>(sums[1][0], params, dc));
                _mm256_storeu_ps(dst + dc + 2 * dstC, Activate<type>(sums[2][0], params, dc));
                _mm256_storeu_ps(dst + dc + 3 * dstC, Activate<type>(sums[3][0], params, dc));
            }
        }

        template<::SimdConvolutionActivationType type> void ConvolutionDirectHwcConvolutionBiasActivationDefault(const float * src, const ConvParam & p, const float * weight, const float * bias, const float * params, float * dst)
        {
            size_t noseH = p.padY, noseW = p.padX;
            size_t bodyH = p.srcH - p.kernelY + 1 + noseH, bodyW = p.srcW - p.kernelX + 1 + noseW;
            size_t tailH = bodyH + p.padH, tailW = bodyW + p.padW;
            size_t bodyW2 = AlignLoAny(bodyW - noseW, 2 * p.strideX) + noseW;
            size_t bodyW4 = AlignLoAny(bodyW - noseW, 4 * p.strideX) + noseW;
            size_t bodyW6 = AlignLoAny(bodyW - noseW, 6 * p.strideX) + noseW;
            size_t wS = p.srcC*p.dstC;
            size_t kY = p.kernelY - noseH, kX = p.kernelX - noseW, kH = bodyH + p.kernelY - 1, kW = bodyW + p.kernelX - 1;
            size_t sy = 0;
            for (; sy < noseH; sy += p.strideY)
            {
                size_t sx = 0;
                const float * w = weight + (noseH - sy) * p.kernelY * wS;
                for (; sx < noseW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src, p, kY + sy, kX + sx, w + (noseW - sx)*wS, bias, params, dst);
                for (; sx < bodyW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src + (sx - noseW) * p.srcC, p, kY + sy, p.kernelX, w, bias, params, dst);
                for (; sx < tailW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src + (sx - noseW) * p.srcC, p, kY + sy, kW - sx, w, bias, params, dst);
            }
            src += (sy - noseH)*p.srcW*p.srcC;
            for (; sy < bodyH; sy += p.strideY)
            {
                size_t sx = 0;
                for (; sx < noseW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src, p, p.kernelY, kX + sx, weight + (noseW - sx)*wS, bias, params, dst);
                if (p.dstC == 24)
                {
                    for (; sx < bodyW4; sx += 4 * p.strideX, dst += 4 * p.dstC)
                        KernelHwcDefaultBody4<type>(src + (sx - noseW) * p.srcC, p, weight, bias, params, dst);
                }
                else
                {
                    for (; sx < bodyW6; sx += 6 * p.strideX, dst += 6 * p.dstC)
                        KernelHwcDefaultBody6<type>(src + (sx - noseW) * p.srcC, p, weight, bias, params, dst);
                }
                for (; sx < bodyW2; sx += 2 * p.strideX, dst += 2 * p.dstC)
                    KernelHwcDefaultBody2<type>(src + (sx - noseW) * p.srcC, p, weight, bias, params, dst);
                for (; sx < bodyW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src + (sx - noseW) * p.srcC, p, p.kernelY, p.kernelX, weight, bias, params, dst);
                for (; sx < tailW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src + (sx - noseW) * p.srcC, p, p.kernelY, kW - sx, weight, bias, params, dst);
                src += p.strideY*p.srcW*p.srcC;
            }
            for (; sy < tailH; sy += p.strideY)
            {
                size_t sx = 0;
                for (; sx < noseW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src, p, kH - sy, kX + sx, weight + (noseW - sx)*wS, bias, params, dst);
                for (; sx < bodyW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src + (sx - noseW) * p.srcC, p, kH - sy, p.kernelX, weight, bias, params, dst);
                for (; sx < tailW; sx += p.strideX, dst += p.dstC)
                    KernelHwcDefaultEdge<type>(src + (sx - noseW) * p.srcC, p, kH - sy, kW - sx, weight, bias, params, dst);
                src += p.strideY*p.srcW*p.srcC;
            }
        }

        template<::SimdConvolutionActivationType type> void ConvolutionDirectHwcConvolutionBiasActivationDepthwise(const float * src, const ConvParam & p, const float * weight, const float * bias, const float * params, float * dst)
        {
            size_t size = p.group;
            size_t sizeF = AlignLo(size, F);
            size_t size2F = AlignLo(size, 2 * F);
            size_t size4F = AlignLo(size, 4 * F);
            size_t size8F = AlignLo(size, 8 * F);
            for (size_t dy = 0; dy < p.dstH; ++dy)
            {
                for (size_t dx = 0; dx < p.dstW; ++dx)
                {
                    size_t i = 0;
                    for (; i < size8F; i += 8 * F)
                    {
                        __m256 sums[8];
                        if (bias)
                        {
                            sums[0] = _mm256_loadu_ps(bias + i + 0 * F);
                            sums[1] = _mm256_loadu_ps(bias + i + 1 * F);
                            sums[2] = _mm256_loadu_ps(bias + i + 2 * F);
                            sums[3] = _mm256_loadu_ps(bias + i + 3 * F);
                            sums[4] = _mm256_loadu_ps(bias + i + 4 * F);
                            sums[5] = _mm256_loadu_ps(bias + i + 5 * F);
                            sums[6] = _mm256_loadu_ps(bias + i + 6 * F);
                            sums[7] = _mm256_loadu_ps(bias + i + 7 * F);
                        }
                        else
                        {
                            sums[0] = _mm256_setzero_ps();
                            sums[1] = _mm256_setzero_ps();
                            sums[2] = _mm256_setzero_ps();
                            sums[3] = _mm256_setzero_ps();
                            sums[4] = _mm256_setzero_ps();
                            sums[5] = _mm256_setzero_ps();
                            sums[6] = _mm256_setzero_ps();
                            sums[7] = _mm256_setzero_ps();
                        }
                        for (size_t ky = 0; ky < p.kernelY; ++ky)
                        {
                            size_t sy = dy * p.strideY + ky * p.dilationY - p.padY;
                            if (sy < p.srcH)
                            {
                                for (size_t kx = 0; kx < p.kernelX; ++kx)
                                {
                                    size_t sx = dx * p.strideX + kx * p.dilationX - p.padX;
                                    if (sx < p.srcW)
                                    {
                                        const float * pw = weight + (ky*p.kernelX + kx)*size + i;
                                        const float * ps = src + (sy*p.srcW + sx)*size + i;
                                        sums[0] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 0 * F), _mm256_loadu_ps(pw + 0 * F)), sums[0]);
                                        sums[1] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 1 * F), _mm256_loadu_ps(pw + 1 * F)), sums[1]);
                                        sums[2] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 2 * F), _mm256_loadu_ps(pw + 2 * F)), sums[2]);
                                        sums[3] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 3 * F), _mm256_loadu_ps(pw + 3 * F)), sums[3]);
                                        sums[4] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 4 * F), _mm256_loadu_ps(pw + 4 * F)), sums[4]);
                                        sums[5] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 5 * F), _mm256_loadu_ps(pw + 5 * F)), sums[5]);
                                        sums[6] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 6 * F), _mm256_loadu_ps(pw + 6 * F)), sums[6]);
                                        sums[7] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 7 * F), _mm256_loadu_ps(pw + 7 * F)), sums[7]);
                                    }
                                }
                            }
                        }
                        _mm256_storeu_ps(dst + i + 0 * F, Activate<type>(sums[0], params, i + 0 * F));
                        _mm256_storeu_ps(dst + i + 1 * F, Activate<type>(sums[1], params, i + 1 * F));
                        _mm256_storeu_ps(dst + i + 2 * F, Activate<type>(sums[2], params, i + 2 * F));
                        _mm256_storeu_ps(dst + i + 3 * F, Activate<type>(sums[3], params, i + 3 * F));
                        _mm256_storeu_ps(dst + i + 4 * F, Activate<type>(sums[4], params, i + 4 * F));
                        _mm256_storeu_ps(dst + i + 5 * F, Activate<type>(sums[5], params, i + 5 * F));
                        _mm256_storeu_ps(dst + i + 6 * F, Activate<type>(sums[6], params, i + 6 * F));
                        _mm256_storeu_ps(dst + i + 7 * F, Activate<type>(sums[7], params, i + 7 * F));
                    }
                    for (; i < size4F; i += 4 * F)
                    {
                        __m256 sums[4];
                        if (bias)
                        {
                            sums[0] = _mm256_loadu_ps(bias + i + 0 * F);
                            sums[1] = _mm256_loadu_ps(bias + i + 1 * F);
                            sums[2] = _mm256_loadu_ps(bias + i + 2 * F);
                            sums[3] = _mm256_loadu_ps(bias + i + 3 * F);
                        }
                        else
                        {
                            sums[0] = _mm256_setzero_ps();
                            sums[1] = _mm256_setzero_ps();
                            sums[2] = _mm256_setzero_ps();
                            sums[3] = _mm256_setzero_ps();
                        }
                        for (size_t ky = 0; ky < p.kernelY; ++ky)
                        {
                            size_t sy = dy * p.strideY + ky * p.dilationY - p.padY;
                            if (sy < p.srcH)
                            {
                                for (size_t kx = 0; kx < p.kernelX; ++kx)
                                {
                                    size_t sx = dx * p.strideX + kx * p.dilationX - p.padX;
                                    if (sx < p.srcW)
                                    {
                                        const float * pw = weight + (ky*p.kernelX + kx)*size + i;
                                        const float * ps = src + (sy*p.srcW + sx)*size + i;
                                        sums[0] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 0 * F), _mm256_loadu_ps(pw + 0 * F)), sums[0]);
                                        sums[1] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 1 * F), _mm256_loadu_ps(pw + 1 * F)), sums[1]);
                                        sums[2] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 2 * F), _mm256_loadu_ps(pw + 2 * F)), sums[2]);
                                        sums[3] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 3 * F), _mm256_loadu_ps(pw + 3 * F)), sums[3]);
                                    }
                                }
                            }
                        }
                        _mm256_storeu_ps(dst + i + 0 * F, Activate<type>(sums[0], params, i + 0 * F));
                        _mm256_storeu_ps(dst + i + 1 * F, Activate<type>(sums[1], params, i + 1 * F));
                        _mm256_storeu_ps(dst + i + 2 * F, Activate<type>(sums[2], params, i + 2 * F));
                        _mm256_storeu_ps(dst + i + 3 * F, Activate<type>(sums[3], params, i + 3 * F));
                    }
                    for (; i < size2F; i += 2 * F)
                    {
                        __m256 sums[2];
                        if (bias)
                        {
                            sums[0] = _mm256_loadu_ps(bias + i + 0 * F);
                            sums[1] = _mm256_loadu_ps(bias + i + 1 * F);
                        }
                        else
                        {
                            sums[0] = _mm256_setzero_ps();
                            sums[1] = _mm256_setzero_ps();
                        }
                        for (size_t ky = 0; ky < p.kernelY; ++ky)
                        {
                            size_t sy = dy * p.strideY + ky * p.dilationY - p.padY;
                            if (sy < p.srcH)
                            {
                                for (size_t kx = 0; kx < p.kernelX; ++kx)
                                {
                                    size_t sx = dx * p.strideX + kx * p.dilationX - p.padX;
                                    if (sx < p.srcW)
                                    {
                                        const float * pw = weight + (ky*p.kernelX + kx)*size + i;
                                        const float * ps = src + (sy*p.srcW + sx)*size + i;
                                        sums[0] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 0 * F), _mm256_loadu_ps(pw + 0 * F)), sums[0]);
                                        sums[1] = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps + 1 * F), _mm256_loadu_ps(pw + 1 * F)), sums[1]);
                                    }
                                }
                            }
                        }
                        _mm256_storeu_ps(dst + i + 0 * F, Activate<type>(sums[0], params, i + 0 * F));
                        _mm256_storeu_ps(dst + i + 1 * F, Activate<type>(sums[1], params, i + 1 * F));
                    }
                    for (; i < size; i += F)
                    {
                        size_t ci = i >= sizeF ? size - F : i;
                        __m256 sum = bias ? _mm256_loadu_ps(bias + ci) : _mm256_setzero_ps();
                        for (size_t ky = 0; ky < p.kernelY; ++ky)
                        {
                            size_t sy = dy * p.strideY + ky * p.dilationY - p.padY;
                            if (sy < p.srcH)
                            {
                                for (size_t kx = 0; kx < p.kernelX; ++kx)
                                {
                                    size_t sx = dx * p.strideX + kx * p.dilationX - p.padX;
                                    if (sx < p.srcW)
                                    {
                                        const float * pw = weight + (ky*p.kernelX + kx)*size + ci;
                                        const float * ps = src + (sy*p.srcW + sx)*size + ci;
                                        sum = _mm256_add_ps(_mm256_mul_ps(_mm256_loadu_ps(ps), _mm256_loadu_ps(pw)), sum);
                                    }
                                }
                            }
                        }
                        _mm256_storeu_ps(dst + ci, Activate<type>(sum, params, ci));
                    }
                    dst += p.dstC;
                }
            }
        }

        template <::SimdConvolutionActivationType type> ConvolutionDirectHwc::ConvolutionBiasActivationPtr GetConvolutionBiasActivation(const ConvParam & p)
        {
            if (p.group == 1)
                return ConvolutionDirectHwcConvolutionBiasActivationDefault<type>;
            else if (p.IsDepthwise())
                return ConvolutionDirectHwcConvolutionBiasActivationDepthwise<type>;
            return NULL;
        }

        ConvolutionDirectHwc::ConvolutionBiasActivationPtr ConvolutionDirectHwc::SetConvolutionBiasActivation()
        {
            const ConvParam & p = _param;
            ConvolutionDirectHwc::ConvolutionBiasActivationPtr func = NULL;
            if (p.dstC >= F && p.dstH >= p.padY + p.padH && p.dstW >= p.padX + p.padW)
            {
                switch (p.activation)
                {
                case ::SimdConvolutionActivationIdentity: func = GetConvolutionBiasActivation<::SimdConvolutionActivationIdentity>(p); break;
                case ::SimdConvolutionActivationRelu: func = GetConvolutionBiasActivation<::SimdConvolutionActivationRelu>(p); break;
                case ::SimdConvolutionActivationLeakyRelu: func = GetConvolutionBiasActivation<::SimdConvolutionActivationLeakyRelu>(p); break;
                case ::SimdConvolutionActivationRestrictRange: func = GetConvolutionBiasActivation<::SimdConvolutionActivationRestrictRange>(p); break;
                case ::SimdConvolutionActivationPrelu: func = GetConvolutionBiasActivation<::SimdConvolutionActivationPrelu>(p); break;
                }
            }
            return func ? func : Sse::ConvolutionDirectHwc::SetConvolutionBiasActivation();
        };

        //---------------------------------------------------------------------

        ConvolutionDepthwiseDotProduct::ConvolutionDepthwiseDotProduct(const ConvParam & p)
            : Sse::ConvolutionDepthwiseDotProduct(p)
        {
        }

        SIMD_INLINE void DotProduct(const float * a, const float * b, size_t offset, __m256 & sum)
        {
            __m256 _a = _mm256_loadu_ps(a + offset);
            __m256 _b = _mm256_loadu_ps(b + offset);
            sum = _mm256_add_ps(_mm256_mul_ps(_a, _b), sum);
        }

        SIMD_INLINE float DotProduct(const float * a, const float * b, size_t size)
        {
            float sum = 0;
            size_t partialAlignedSize = AlignLo(size, F);
            size_t fullAlignedSize = AlignLo(size, QF);
            size_t i = 0;
            if (partialAlignedSize)
            {
                __m256 sums[4] = { _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps(), _mm256_setzero_ps() };
                if (fullAlignedSize)
                {
                    for (; i < fullAlignedSize; i += QF)
                    {
                        DotProduct(a, b, i + F * 0, sums[0]);
                        DotProduct(a, b, i + F * 1, sums[1]);
                        DotProduct(a, b, i + F * 2, sums[2]);
                        DotProduct(a, b, i + F * 3, sums[3]);
                    }
                    sums[0] = _mm256_add_ps(_mm256_add_ps(sums[0], sums[1]), _mm256_add_ps(sums[2], sums[3]));
                }
                for (; i < partialAlignedSize; i += F)
                    DotProduct(a, b, i, sums[0]);
                sum += ExtractSum(sums[0]);
            }
            for (; i < size; ++i)
                sum += a[i] * b[i];
            return sum;
        }

        void ConvolutionDepthwiseDotProduct::Forward(const float * src, float * buf, float * dst)
        {
            if (_bias)
            {
                for (size_t i = 0; i < _count; ++i)
                    dst[i] = DotProduct(src + i * _size, _weight + i * _size, _size) + _bias[i];
            }
            else
            {
                for (size_t i = 0; i < _count; ++i)
                    dst[i] = DotProduct(src + i * _size, _weight + i * _size, _size);
            }
            if (_param.activation)
                ConvolutionBiasAndActivation(NULL, _count, 1, _param.activation, _params, ::SimdFalse, dst);
        }

        //---------------------------------------------------------------------

        void * ConvolutionInit(size_t srcC, size_t srcH, size_t srcW, SimdBool srcT, size_t dstC, SimdBool dstT,
            size_t kernelY, size_t kernelX, size_t dilationY, size_t dilationX, size_t strideY, size_t strideX,
            size_t padY, size_t padX, size_t padH, size_t padW, size_t group, SimdConvolutionActivationType activation, SimdGemm32fNNPtr gemm)
        {
            ConvParam param(srcC, srcH, srcW, srcT, dstC, dstT, kernelY, kernelX, dilationY, dilationX, strideY, strideX, padY, padX, padH, padW, group, activation, gemm);
            if (!param.Valid())
                return NULL;
            else if (ConvolutionDepthwiseDotProduct::Preferable(param))
                return new ConvolutionDepthwiseDotProduct(param);
            else if (ConvolutionWinograd2x3p::Preferable(param))
                return new ConvolutionWinograd2x3p(param);
            else if (ConvolutionGemmNT::Preferable(param))
                return new ConvolutionGemmNT(param);
            else if (ConvolutionDirectChw::Preferable(param))
                return new Avx::ConvolutionDirectChw(param);
            else if (ConvolutionDirectHwc::Preferable(param))
                return new ConvolutionDirectHwc(param);
            else
                return new ConvolutionGemmNN(param);
        }
    }
#endif//SIMD_AVX_ENABLE
}
