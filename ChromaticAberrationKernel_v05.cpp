// ChromaticAberrationKernel v1.0
// by Lukas Schwabe - Mackevision - 01-2019
// fbm_Noise_blink_kernel v1.0 by Johannes Saam - Nukepedia


kernel ChromaticAberrationKernel : ImageComputationKernel<ePixelWise> {

    Image<eRead, eAccessRanged2D, eEdgeClamped> src;
    Image<eWrite> dst;

    param:
    bool smear;
    bool trueChroma;
    bool calcAlpha;

    // float kernelSize;
    int2 res;
    int2 effectCenter;

    float centerSize;

    float chromaMult;

    float2 chromaRed;
    float2 chromaGreen;
    float2 chromaBlue;

    float chromaRotateMult;

    float chromaMix;

    float4 redChannel;
    float4 greenChannel;
    float4 blueChannel;

    // noise vals
    float octaves;
    float gain;
    float lacunarity;
    float gamma;
    float zz;
    float amplitude;
    float3 offset;
    float3 scale;

    float noiseOverDistance;



    local:
    int2 _bokehOffset;
    float2 center;
    int res_max;

    float chromaMixClamped;
    float centerSizeClamped;
    float sChromaMult;
    float sChromaRotateMult;
    float gammaClamped;


    // noise vals
    int m_width;
    int m_height;

    float noiseMult;


    void define() {

        defineParam(smear, "Smear Chroma", true);
        defineParam(trueChroma, "True Chroma Blur", true);

        defineParam(res, "Resolution", int2(1920, 1080));
        defineParam(effectCenter, "Center", int2(960, 540));

        defineParam(centerSize, "Center Size", 1.0f);
        defineParam(chromaMult, "Chroma Mult", 10.0f);

        defineParam(chromaRed, "Chroma Red Mult", float2(1.0f,1.0f));
        defineParam(chromaGreen, "Chroma Green Mult", float2(1.0f,1.0f));
        defineParam(chromaBlue, "Chroma Blue Mult", float2(1.0f,1.0f));

        defineParam(chromaRotateMult, "Chroma Rotation", 0.0f);

        defineParam(chromaMix, "Chroma Mix", 1.0f);

        defineParam(redChannel, "Red Balance", float4(1.0f,0.0f,0.0f,0.0f));
        defineParam(greenChannel, "Green Balance", float4(0.0f,1.0f,0.0f,0.0f));
        defineParam(blueChannel, "Blue Balance", float4(0.0f,0.0f,1.0f,0.0f));

        // noise vals
        defineParam(octaves, "Octaves", 10.0f);
        defineParam(gain, "Gain", 0.5f);
        defineParam(lacunarity, "Lacunarity", 2.0f);
        defineParam(gamma, "Gamma", 0.5f);
        defineParam(amplitude, "amplitude", 0.5f);
        defineParam(zz, "Z", 50.0f);
        defineParam(offset, "Offset", float3(0.0f, 0.0f, 0.0f));
        defineParam(scale, "Scale", float3(0.001f, 0.001f, 0.001f));
        defineParam(noiseOverDistance, "noiseOverDistance", 1.0f);
    }

    void init() {

        // avoid user values that will crash
        chromaMixClamped = clamp(chromaMix,0.0f,1.0f);
        centerSizeClamped = centerSize<0.0f?0.0f:centerSize;
        noiseMult = max(noiseOverDistance,0.0f);
        gammaClamped = clamp(gammaClamped, 0.2f,2.0f);

        // max size
        res_max = max(res.x,res.y);

        // bring chroma Mults to reasonable values
        sChromaMult = chromaMult / 300;
        sChromaRotateMult = -chromaRotateMult / 200;

        src.setRange(-res_max, res_max);

        // set center point
        center.x = effectCenter.x;
        center.y = effectCenter.y;

        m_width = dst.bounds.width();
        m_height = dst.bounds.height();

    }

    // Chromatic Aberration Kernel
    void process(int2 pos) {

        SampleType(src) chromaDst(0);
        SampleType(src) chromaDstEffected(0);

        float2 offset;
        float2 direction;
        float distanceToCenter;
        float nDistanceToCenter;
        float sDistanceToCenter;

        float2 posRed;
        float2 posGreen;
        float2 posBlue;

        offset.x = (float)pos.x-center.x;
        offset.y = (float)pos.y-center.y;

        if (offset.x == 0.0f && offset.y == 0.0f) {
            direction = 0.0f;
            distanceToCenter = 0.0f;
        }
        else {
            direction = normalize(offset);
            distanceToCenter = length(offset);
        }

        if (distanceToCenter >= 1.0f) {
            nDistanceToCenter = distanceToCenter/res_max;
            sDistanceToCenter = pow(nDistanceToCenter,centerSizeClamped);
        }
        else {
            nDistanceToCenter = 0.0f;
            sDistanceToCenter = 0.0f;
        }


        //generate sin and cos for rotation
        float cs = cos(3.1415926535f/180.0f);
        float sn = sin(3.1415926535f/180.0f);

        // calc aberrated positions with scale and rotation and noise
        float getThatNoise = getNoise(pos,sDistanceToCenter);
        float2 noise;

        noise.x = getThatNoise*-direction.x;
        noise.y = getThatNoise*-direction.y;

        posRed.x =-distanceToCenter * sDistanceToCenter * direction.x * sChromaMult * chromaRed.x + ((sn*offset.x + cs*offset.y) * sDistanceToCenter * sChromaRotateMult * chromaRed.x )+noise.x;
        posRed.y =-distanceToCenter * sDistanceToCenter * direction.y * sChromaMult * chromaRed.y + ((-cs*offset.x + sn*offset.y) * sDistanceToCenter * sChromaRotateMult * chromaRed.y)+noise.y;

        posGreen.x = -distanceToCenter * sDistanceToCenter * direction.x * sChromaMult * chromaGreen.x + ((sn*offset.x + cs*offset.y) * sDistanceToCenter * sChromaRotateMult * chromaGreen.x )+noise.x;
        posGreen.y = -distanceToCenter * sDistanceToCenter * direction.y * sChromaMult * chromaGreen.y + ((-cs*offset.x + sn*offset.y) * sDistanceToCenter * sChromaRotateMult * chromaGreen.y)+noise.y;

        posBlue.x = -distanceToCenter * sDistanceToCenter * direction.x * sChromaMult * chromaBlue.x + ((sn*offset.x + cs*offset.y) * sDistanceToCenter * sChromaRotateMult * chromaBlue.x )+noise.x;
        posBlue.y = -distanceToCenter * sDistanceToCenter * direction.y * sChromaMult * chromaBlue.y + ((-cs*offset.x + sn*offset.y) * sDistanceToCenter * sChromaRotateMult * chromaBlue.y)+noise.y;

        float lengthRed = length(posRed);
        float lengthGreen = length(posGreen);
        float lengthBlue = length(posBlue);

        float filterStart = 0.0f;



        if (smear == true) { //smear


            float stretchRed = 0;
            float stretchGreen = 0;
            float stretchBlue = 0;
            float stretchAlpha = 0;


            if (trueChroma == true) { //true chroma mode

                int counterRed = (int)fabs(lengthRed)+1;
                int counterGreen = (int)fabs(lengthGreen)+1;
                int counterBlue = (int)fabs(lengthBlue)+1;

                //red
                if (lengthRed > filterStart) {

                    int loopRuns = 0;
                    stretchAlpha = 0;
                    for (int count=0 ; count <= counterRed; count++) {

                        float stepValue = (float)count/(float)counterRed;

                        stretchRed += bilinear(src, posRed.x*stepValue , posRed.y*stepValue, 0) * stepValue;
                        if (stepValue != 1) {
                          stretchRed += bilinear(src, posRed.x*2-posRed.x*stepValue , posRed.y*2-posRed.y*stepValue, 0) * stepValue;
                        }

                        if (calcAlpha == true) {
                            stretchAlpha += bilinear(src, posRed.x*stepValue , posRed.y*stepValue, 3) * stepValue;
                            if (stepValue != 1) {
                              stretchAlpha += bilinear(src, posRed.x*2-posRed.x*stepValue , posRed.y*2-posRed.y*stepValue, 3) * stepValue;
                            }
                        }


                        loopRuns +=1;
                    }


                    chromaDstEffected.x += stretchRed / (loopRuns-1);
                    if (calcAlpha == true) {
                        chromaDstEffected.w += (stretchAlpha / (loopRuns-1))*0.299f;
                    }
                }
                else {
                    chromaDst.x += bilinear(src, posRed.x, posRed.y, 0);
                    if (calcAlpha == true) {
                        chromaDst.w += bilinear(src, posRed.x, posRed.y, 3)*0.299f;
                    }
                }


                //green
                if (lengthGreen > filterStart) {

                    int loopRuns = 0;
                    stretchAlpha = 0;
                    for (int count=0 ; count <= counterGreen; count++) {

                        float stepValue = (float)count/(float)counterGreen;

                        stretchGreen +=  bilinear(src, posGreen.x*stepValue+posRed.x, posGreen.y*stepValue+posRed.y, 1) * stepValue;
                        if (stepValue != 1) {
                            stretchGreen += bilinear(src, posGreen.x*2-posGreen.x*stepValue+posRed.x , posGreen.y*2-posGreen.y*stepValue+posRed.y, 1) * stepValue;
                        }

                        if (calcAlpha == true) {
                            stretchAlpha +=  bilinear(src, posGreen.x*stepValue+posRed.x, posGreen.y*stepValue+posRed.y, 3) * stepValue;
                            if (stepValue != 1) {
                                stretchAlpha += bilinear(src, posGreen.x*2-posGreen.x*stepValue+posRed.x , posGreen.y*2-posGreen.y*stepValue+posRed.y, 3) * stepValue;
                            }
                        }

                        loopRuns +=1;
                    }


                    chromaDstEffected.y += stretchGreen / (loopRuns-1);
                    if (calcAlpha == true) {
                        chromaDstEffected.w += (stretchAlpha / (loopRuns-1))*0.587f;
                    }
                }

                else {
                    chromaDst.y += bilinear(src, posGreen.x, posGreen.y, 1);
                    if (calcAlpha == true) {
                        chromaDst.w += bilinear(src, posGreen.x, posGreen.y, 3)*0.587f;
                    }

                }


                //blue
                if (lengthBlue > filterStart) {

                    int loopRuns = 0;
                    stretchAlpha = 0;
                    for (int count=0 ; count <= counterBlue; count++) {

                        float stepValue = (float)count/(float)counterBlue;

                        stretchBlue += bilinear(src, posBlue.x*stepValue+posGreen.x+posRed.x , posBlue.y*stepValue+posGreen.y+posRed.y, 2) * stepValue ;
                        if (stepValue != 1) {
                            stretchBlue += bilinear(src, posBlue.x*2-posBlue.x*stepValue+posGreen.x+posRed.x , posBlue.y*2-posBlue.y*stepValue+posGreen.y+posRed.y, 2) * stepValue;
                        }

                        if (calcAlpha == true) {
                            stretchAlpha += bilinear(src, posBlue.x*stepValue+posGreen.x+posRed.x , posBlue.y*stepValue+posGreen.y+posRed.y, 3) * stepValue ;
                            if (stepValue != 1) {
                                stretchAlpha += bilinear(src, posBlue.x*2-posBlue.x*stepValue+posGreen.x+posRed.x , posBlue.y*2-posBlue.y*stepValue+posGreen.y+posRed.y, 3) * stepValue;
                            }
                        }

                        loopRuns +=1;

                    }


                    chromaDstEffected.z += stretchBlue / (loopRuns-1);
                    if (calcAlpha == true) {
                        chromaDstEffected.w += (stretchAlpha / (loopRuns-1))*0.114f;
                    }
                }

                else {
                    chromaDst.z += bilinear(src, posBlue.x, posBlue.y, 2);
                    if (calcAlpha == true) {
                        chromaDst.w += bilinear(src, posBlue.x, posBlue.y, 3)*0.114f;
                    }
                }

            } //true chroma mode end








            else{  // non true chroma mode
                int counterRed = (int)fabs(lengthRed)+1;
                int counterGreen = (int)fabs(lengthGreen)+1;
                int counterBlue = (int)fabs(lengthBlue)+1;


                //red
                if (lengthRed > filterStart) {

                    int loopRuns = 0;
                    stretchAlpha = 0;
                    for (int count=0 ; count <= counterRed; count++) {

                        float stepValue = (float)count/(float)counterRed;


                        stretchRed += bilinear(src, (posRed.x-noise.x*noiseMult)*stepValue+noise.x*noiseMult*stepValue*0.5f, (posRed.y-noise.y*noiseMult)*stepValue + noise.y*noiseMult*stepValue*0.5f, 0) * stepValue;
                        if (stepValue != 1) {

                            stretchRed += bilinear(src, posRed.x*2-posRed.x*stepValue-noise.x*noiseMult*stepValue*0.5f, posRed.y*2-posRed.y*stepValue-noise.y*noiseMult*stepValue*0.5f, 0) * stepValue;
                        }

                        if (calcAlpha == true) {
                            stretchAlpha += bilinear(src, (posRed.x-noise.x*noiseMult)*stepValue+noise.x*noiseMult*stepValue*0.5f, (posRed.y-noise.y*noiseMult)*stepValue + noise.y*noiseMult*stepValue*0.5f, 3) * stepValue;
                            if (stepValue != 1) {

                                stretchAlpha += bilinear(src, posRed.x*2-posRed.x*stepValue-noise.x*noiseMult*stepValue*0.5f, posRed.y*2-posRed.y*stepValue-noise.y*noiseMult*stepValue*0.5f, 3) * stepValue;
                            }
                        }

                        loopRuns +=1;
                    }


                    chromaDstEffected.x += stretchRed / (loopRuns-1);
                    if (calcAlpha == true) {
                        chromaDstEffected.w += (stretchAlpha / (loopRuns-1))*0.299f;
                    }
                }
                else {
                    chromaDst.x += bilinear(src, posRed.x, posRed.y, 0);
                    if (calcAlpha == true) {
                        chromaDst.w += bilinear(src, posRed.x, posRed.y, 3)*0.299f;
                    }
                }


                //green
                if (lengthGreen > filterStart) {

                    int loopRuns = 0;
                    stretchAlpha = 0;
                    for (int count=0 ; count <= counterGreen; count++) {

                        float stepValue = (float)count/(float)counterGreen;

                        stretchGreen +=  bilinear(src, (posGreen.x-noise.x*noiseMult)*stepValue+noise.x*noiseMult*stepValue*0.5f, (posGreen.y-noise.y*noiseMult)*stepValue + noise.y*noiseMult*stepValue*0.5f, 1) * stepValue;
                        if (stepValue != 1) {
                            stretchGreen += bilinear(src, posGreen.x*2-posGreen.x*stepValue-noise.x*noiseMult*stepValue*0.5f, posGreen.y*2-posGreen.y*stepValue-noise.y*noiseMult*stepValue*0.5f, 1) * stepValue;
                        }

                        if (calcAlpha == true) {
                            stretchAlpha +=  bilinear(src, (posGreen.x-noise.x*noiseMult)*stepValue+noise.x*noiseMult*stepValue*0.5f, (posGreen.y-noise.y*noiseMult)*stepValue + noise.y*noiseMult*stepValue*0.5f, 3) * stepValue;
                            if (stepValue != 1) {
                                stretchAlpha += bilinear(src, posGreen.x*2-posGreen.x*stepValue-noise.x*noiseMult*stepValue*0.5f, posGreen.y*2-posGreen.y*stepValue-noise.y*noiseMult*stepValue*0.5f, 3) * stepValue;
                            }
                        }
                        loopRuns +=1;
                    }


                    chromaDstEffected.y += stretchGreen / (loopRuns-1);
                    if (calcAlpha == true) {
                        chromaDstEffected.w += (stretchAlpha / (loopRuns-1))*0.587f;
                    }
                }

                else {
                    chromaDst.y += bilinear(src, posGreen.x, posGreen.y, 1);
                    if (calcAlpha == true) {
                        chromaDst.w += bilinear(src, posGreen.x, posGreen.y, 3)*0.587f;
                    }

                }


                //blue
                if (lengthBlue > filterStart) {

                    int loopRuns = 0;
                    stretchAlpha = 0;
                    for (int count=0 ; count <= counterBlue; count++) {

                        float stepValue = (float)count/(float)counterBlue;

                        stretchBlue += bilinear(src, (posBlue.x-noise.x*noiseMult)*stepValue+noise.x*noiseMult*stepValue*0.5f, (posBlue.y-noise.y*noiseMult)*stepValue + noise.y*noiseMult*stepValue*0.5f, 2) * stepValue;
                        if (stepValue != 1) {
                            stretchBlue += bilinear(src, posBlue.x*2-posBlue.x*stepValue-noise.x*noiseMult*stepValue*0.5f, posBlue.y*2-posBlue.y*stepValue-noise.y*noiseMult*stepValue*0.5f, 2) * stepValue;
                        }

                        if (calcAlpha == true) {
                            stretchAlpha += bilinear(src, (posBlue.x-noise.x*noiseMult)*stepValue+noise.x*noiseMult*stepValue*0.5f, (posBlue.y-noise.y*noiseMult)*stepValue + noise.y*noiseMult*stepValue*0.5f, 3) * stepValue;
                            if (stepValue != 1) {
                                stretchAlpha += bilinear(src, posBlue.x*2-posBlue.x*stepValue-noise.x*noiseMult*stepValue*0.5f, posBlue.y*2-posBlue.y*stepValue-noise.y*noiseMult*stepValue*0.5f, 3) * stepValue;
                            }
                        }

                        loopRuns +=1;
                    }


                    chromaDstEffected.z += stretchBlue / (loopRuns-1);
                    if (calcAlpha == true) {
                        chromaDstEffected.w += (stretchAlpha / (loopRuns-1))*0.114f;
                    }

                }

                else {
                    chromaDst.z += bilinear(src, posBlue.x, posBlue.y, 2);
                    if (calcAlpha == true) {
                        chromaDst.w += bilinear(src, posBlue.x, posBlue.y, 3)*0.114f;
                    }
                }



            } //non true chroma mode end

        } // end smear


        else {  //no smear

            if (trueChroma == true) {
                posGreen += posRed;
                posBlue += posGreen;
            }

            if (lengthRed>filterStart) {
                chromaDstEffected.x += bilinear(src, posRed.x, posRed.y, 0);
            }
            else {
                chromaDst.x = bilinear(src, posRed.x, posRed.y, 0);
            }


            if (lengthGreen>filterStart) {
                chromaDstEffected.y += bilinear(src, posGreen.x, posGreen.y, 1);
            }
            else {
                chromaDst.y = bilinear(src, posGreen.x, posGreen.y, 1);
            }


            if (lengthBlue>filterStart) {
                chromaDstEffected.z += bilinear(src, posBlue.x, posBlue.y, 2);
            }
            else {
                chromaDst.z= bilinear(src, posBlue.x, posBlue.y, 2);
            }

            if (calcAlpha == true) {
                if (lengthRed>filterStart || lengthGreen>filterStart || lengthBlue>filterStart) {
                    chromaDstEffected.w = bilinear(src, posRed.x, posRed.y, 3)*0.299f + bilinear(src, posGreen.x, posGreen.y, 3)*0.587f + bilinear(src, posBlue.x, posBlue.y, 3)*0.114f;
                }
            }


        } // no smear end


        // add up and hue shift effected pixels
        chromaDst.x = chromaDst.x + chromaDstEffected.x*redChannel.x + chromaDstEffected.y*greenChannel.x*lengthGreen + chromaDstEffected.z*blueChannel.x*lengthBlue;
        chromaDst.y = chromaDst.y + chromaDstEffected.x*redChannel.y*lengthRed + chromaDstEffected.y*greenChannel.y + chromaDstEffected.z*blueChannel.y*lengthBlue;
        chromaDst.z = chromaDst.z + chromaDstEffected.x*redChannel.z*lengthRed + chromaDstEffected.y*greenChannel.z*lengthGreen + chromaDstEffected.z*blueChannel.z;

        // add up alpha
        if (calcAlpha == true) {
            chromaDst.w = chromaDst.w + chromaDstEffected.w;
        }
        else {
            chromaDst.w = src(0,0,3);
        }

        //Mix Values with Original
        chromaDst.x = chromaDst.x * chromaMixClamped + src(0,0,0) * (1.0f-chromaMixClamped);
        chromaDst.y = chromaDst.y * chromaMixClamped + src(0,0,1) * (1.0f-chromaMixClamped);
        chromaDst.z = chromaDst.z * chromaMixClamped + src(0,0,2) * (1.0f-chromaMixClamped);
        if (calcAlpha == true) {
            chromaDst.w = chromaDst.w * chromaMixClamped + src(0,0,3) * (1.0f-chromaMixClamped);
        }

        //DEBUG
        // chromaDst.x = 1.0f;


        dst() = chromaDst;

    }
    // Chromatic Aberration Kernel end


    // simplified noise call for Chromatic Aberration
    inline float getNoise(int2 pos, float amplitudeFromCenter) {
        float noiseVal;

        noiseVal = pow( fbm_noise_3d( octaves, gain, lacunarity, ( ((float)pos.x) * scale.x ) + offset.x, ( ((float)pos.y) * scale.y ) + offset.y, ( zz * scale.z ) + offset.z ) * amplitude * amplitudeFromCenter, 1.0f / gammaClamped );

        return noiseVal;
    }


    // noise
    // fbm_Noise_blink_kernel v1.0 by Johannes Saam - Nukepedia

    // Permutation table.  The same list is repeated twice.
    inline int perm( int index )
    {
        int permData[512] = {
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
            8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
            35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
            134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
            55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
            250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
            189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
            172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
            228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
            107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
            138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
            8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
            35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
            134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
            55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
            250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
            189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
            172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
            228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
            107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
            138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
        };
        return permData[index];
    };

    // The gradients are the midpoints of the vertices of a cube.
    inline float3 grad3( int index )
    {
        float grad3Data[12*3] = { 1.0f,1.0f,0.0f,-1.0f,1.0f,0.0f,1.0f,-1.0f,0.0f,-1.0f,-1.0f,0.0f,1.0f,0.0f,1.0f,-1.0f,0.0f,1.0f,1.0f,0.0f,-1.0f,-1.0f,0.0f,-1.0f,0.0f,1.0f,1.0f,0.0f,-1.0f,1.0f,0.0f,1.0f,-1.0f,0.0f,-1.0f,-1.0f };

        return (float3)(grad3Data[index*3], grad3Data[(index*3) + 1], grad3Data[(index*3) + 2]);
    };

    inline float dotNoise( float3 g, float x, float y, float z )
    {
        return g.x*x + g.y*y + g.z*z;
    };

    inline int fastfloor( float x )
    {
        if( x > 0.0f )
        {
            return (int)(x);
        }
        else
        {
            return (int)(x - 1);
        }
    };

    // 3D raw Simplex noise
    inline float raw_noise_3d(		float x, float y, float z ) {
        float n0, n1, n2, n3; // Noise contributions from the four corners

        // Skew the input space to determine which simplex cell we're in
        float F3 = 1.0f/3.0f;
        float s = (x+y+z)*F3; // Very nice and simple skew factor for 3D
        int i = fastfloor(x+s);
        int j = fastfloor(y+s);
        int k = fastfloor(z+s);

        float G3 = 1.0f/6.0f; // Very nice and simple unskew factor, too
        float t = (i+j+k)*G3;
        float X0 = i-t; // Unskew the cell origin back to (x,y,z) space
        float Y0 = j-t;
        float Z0 = k-t;
        float x0 = x-X0; // The x,y,z distances from the cell origin
        float y0 = y-Y0;
        float z0 = z-Z0;

        // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
        // Determine which simplex we are in.
        int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
        int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

        if( x0 >= y0 )
        {
            if( y0>=z0 )
            {
                i1=1; j1=0; k1=0; i2=1; j2=1; k2=0;
            } // X Y Z order
            else if( x0 >= z0 )
            {
                i1=1; j1=0; k1=0; i2=1; j2=0; k2=1;
            } // X Z Y order
            else
            {
                i1=0; j1=0; k1=1; i2=1; j2=0; k2=1;
            } // Z X Y order
        }
        else
        {   // x0<y0
            if(y0<z0) {
                i1=0; j1=0; k1=1; i2=0; j2=1; k2=1;
            } // Z Y X order
            else if(x0<z0) {
                i1=0; j1=1; k1=0; i2=0; j2=1; k2=1;
            } // Y Z X order
            else {
                i1=0; j1=1; k1=0; i2=1; j2=1; k2=0;
            } // Y X Z order
        }

        // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
        // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
        // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
        // c = 1/6.
        float x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
        float y1 = y0 - j1 + G3;
        float z1 = z0 - k1 + G3;
        float x2 = x0 - i2 + 2.0*G3; // Offsets for third corner in (x,y,z) coords
        float y2 = y0 - j2 + 2.0*G3;
        float z2 = z0 - k2 + 2.0*G3;
        float x3 = x0 - 1.0 + 3.0*G3; // Offsets for last corner in (x,y,z) coords
        float y3 = y0 - 1.0 + 3.0*G3;
        float z3 = z0 - 1.0 + 3.0*G3;

        // Work out the hashed gradient indices of the four simplex corners
        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;
        int gi0 = perm(ii+perm(jj+perm(kk))) % 12;
        int gi1 = perm(ii+i1+perm(jj+j1+perm(kk+k1))) % 12;
        int gi2 = perm(ii+i2+perm(jj+j2+perm(kk+k2))) % 12;
        int gi3 = perm(ii+1+perm(jj+1+perm(kk+1))) % 12;

        // Calculate the contribution from the four corners
        float t0 = 0.6 - x0*x0 - y0*y0 - z0*z0;
        if( t0 < 0 )
        {
            n0 = 0.0;
        }
        else
        {
            t0 *= t0;
            n0 = t0 * t0 * dotNoise(grad3(gi0), x0, y0, z0);
        }

        float t1 = 0.6 - x1*x1 - y1*y1 - z1*z1;
        if( t1<0 )
        {
            n1 = 0.0;
        }
        else
        {
            t1 *= t1;
            n1 = t1 * t1 * dotNoise(grad3(gi1), x1, y1, z1);
        }

        float t2 = 0.6 - x2*x2 - y2*y2 - z2*z2;
        if( t2 < 0 )
        {
            n2 = 0.0;
        }
        else
        {
            t2 *= t2;
            n2 = t2 * t2 * dotNoise(grad3(gi2), x2, y2, z2);
        }

        float t3 = 0.6 - x3*x3 - y3*y3 - z3*z3;
        if( t3 < 0 )
        {
            n3 = 0.0;
        }
        else {
            t3 *= t3;
            n3 = t3 * t3 * dotNoise(grad3(gi3), x3, y3, z3);
        }

        // Add contributions from each corner to get the final noise value.
        // The result is scaled to stay just inside [-1,1]
        return 32.0*(n0 + n1 + n2 + n3);
    };

    // 3D Multi-octave Simplex noise.
    //
    // For each octave, a higher frequency/lower amplitude function will be added to the original.
    // The higher the persistence [0-1], the more of each succeeding octave will be added.
    inline float octave_noise_3d(	float octaves, float persistence, float scale, float x, float y, float z ) {
        float total = 0;
        float frequency = scale;
        float amplitude = 1;

        // We have to keep track of the largest possible amplitude,
        // because each octave adds more, and we need a value in [-1, 1].
        float maxAmplitude = 0;
        int i = 0;
        for( i=0; i < octaves; i++ )
        {
            total += raw_noise_3d( x * frequency, y * frequency, z * frequency ) * amplitude;

            frequency *= 2.0f;
            maxAmplitude += amplitude;
            amplitude *= persistence;
        }

        return total / maxAmplitude;
    };

    inline float fbm_noise_3d(	float octaves, float gain, float lacunarity, float x, float y, float z ) {

        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = gain;

        int i = 0;
        for ( i = 0; i < octaves; i++ )
        {
            total += raw_noise_3d( x * frequency, y * frequency, z * frequency ) * amplitude;
            frequency *= lacunarity;
            amplitude *= gain;
        }

        return total / 2.0f + 1.0f;
    };
    // fbm_Noise_blink_kernel v1.0 by Johannes Saam ends here

};
