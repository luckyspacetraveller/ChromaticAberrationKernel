kernel Bokeh : public ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRanged2D, eEdgeClamped> src;
  Image<eRead, eAccessRanged2D, eEdgeClamped> depth;
  Image<eRead, eAccessRandom> bokeh;
  Image<eWrite> result;

local:
  int2 bokehOffset;
  float bokehMax;
  int convolveSize;
  int min_convolveSize;
  float res_max;

  int maxSlice;
  int minSlice;

param:
  int2 res;

  float CatEye_Strength;
  float CatEye_Scale;
  float maxDepthSize;
  float minDepthSize;

  int clampSlice;

  int TestSlice;


  void define() {
    defineParam(CatEye_Strength, "strength", 1.0f);
    defineParam(CatEye_Scale, "Cat",  1.0f);

    defineParam(maxDepthSize, "Maximum Depth Size", 10.0f);
	defineParam(minDepthSize, "Minimum Depth Size", -10.0f);

	defineParam(TestSlice, "TestSlice", 1);

	defineParam(res, "Resolution", int2(1920, 1080));

	defineParam(clampSlice, "clampSlice",  1);
  }

  void init(){
	convolveSize = (int)ceil(max(maxDepthSize,fabs(minDepthSize)));

	maxSlice = ceil(maxDepthSize);
	minSlice = floor(minDepthSize);

    //Store the offset of the bottom-left corner of the filter image
    bokehOffset = (bokeh.bounds.width()/2, bokeh.bounds.height()/2);
	bokehMax = max(bokeh.bounds.width(), bokeh.bounds.height());

	//max size
	res_max = max(res.y,res.x);

	//source Image range
    src.setRange(-convolveSize, convolveSize);
	depth.setRange(-convolveSize, convolveSize);
  }

  void process(int2 pos) {

	// set rgba
	SampleType(src) value(0);

	//bokeh
	value = bokeh_filter(pos);

	//write back
    result() = value;
  }

  float4 bokeh_filter(int2 pos) {

	// CatEye
	float2 Cat;
	float2 offset;
	float2 direction;
	float strength;

	//Bokeh
    SampleType(src) valueSum(0);
    ValueType(bokeh) bokehSum(0);
	ValueType(bokeh) AlphaSum(0);


	//Depth values
	float zTarValue(0);
	float zValue = floor(clamp((depth(0,0,0)), minDepthSize, maxDepthSize));

	//Bokeh Position
	float2 xyBokeh;

	//Distance to Corner
	float CornerDist = length((float2)(res.x/res_max,res.y/res_max));

	//Bokeh max Corner
	float CatEyeCornerDist = length((float2)(bokeh.bounds.width()/bokehMax,bokeh.bounds.height()/bokehMax));

	//CatEyeSize
	float CatEye_Size = CatEyeCornerDist * CatEye_Scale;


	// Pixel Position
	offset.x = (float)(pos.x *2  - res.x) /res_max;
	offset.y = (float)(pos.y *2  - res.y) /res_max;

	// Relative Position + Gradient
	direction = normalize(offset);
	strength = fabs(length(offset)) / CornerDist ;

	// is sharp?
	int inFocus;

	//Color Stuff
	float4 RGB;
	SampleType(src) combine(0);
	SampleType(src) test(0);

	// for(int s = TestSlice; s == TestSlice; s++) {
	for(int s = minSlice; s <= maxSlice - clampSlice; s++) {

		//current Slice
		int CurSlice = fabs(s);

		//reset for next slice
		float  alphaHit = 0;
		float  alphaNoHit = 0;
		float alpha = 0;
		int setColor = 0;

		valueSum = 0;
		bokehSum = 0;

		float4 bokeh_Color;
		float bokeh_val;

		//Blur 0 ?
		if(zValue == 0 && s == 0 ){
			inFocus =1;
		}

		else{

			inFocus = 0;

			for(int y = -CurSlice; y < CurSlice; y++) {
				for(int x = -CurSlice; x < CurSlice; x++) {

      				if (s <= 0) { // focus to infinity

                //Load Target Depth value
      					zTarValue = clamp(depth(x,y,0), minDepthSize, maxDepthSize);

      					 if (fabs(zTarValue) > x || fabs(zTarValue) > y){
      						// Scale Bokeh
      						xyBokeh.x = (((float)(x) / CurSlice *-1) / (fabs(zTarValue)/CurSlice) +1) *  (bokehOffset.x);
      						xyBokeh.y = (((float)(y) / CurSlice *-1) / (fabs(zTarValue)/CurSlice) +1) *  (bokehOffset.y);

      						//Clamp max Bokeh position value
      						xyBokeh.x = clamp(xyBokeh.x, 0.0f,(float)(bokeh.bounds.width()));
      						xyBokeh.y = clamp(xyBokeh.y, 0.0f,(float)(bokeh.bounds.height()));

      						bokeh_Color = bilinear(bokeh, xyBokeh.x, xyBokeh.y);

      						bokeh_val = (bokeh_Color.x*0.299   + bokeh_Color.y *0.587 + bokeh_Color.z*0.114   	);

      						if(bokeh_val > 0){
      							if(zTarValue -  s >= 0){


      								Cat.x = ((float)(x) / CurSlice)/ (fabs(zTarValue)/CurSlice) + (direction.x * strength * CatEye_Strength* CatEye_Size *  CornerDist );
      								Cat.y = ((float)(y) / CurSlice)/ (fabs(zTarValue)/CurSlice) + (direction.y * strength * CatEye_Strength* CatEye_Size *  CornerDist );

      								if(length(Cat) < CatEye_Size){
      									if(zTarValue -  s < 1){



      										if (zTarValue -  s < 1){

      											valueSum += bokeh_Color * src(x,y);
      										}

      										else{

      											valueSum += bokeh_Color * src(0,0);
      										}

      										bokehSum += bokeh_val;
      										alphaHit += 1;
      									}
      								}

      								else{
      									alphaNoHit += 1;
      								}
      							}
      							else{
      								alphaNoHit += 1;
      							}
      						}
      					}
                  } // end focus to infinity




                  else { // cam to focus


                    //Load Target Depth value
      					zTarValue = clamp(depth(x,y,0), minDepthSize, maxDepthSize);

      					 // if (fabs(zTarValue) > x || fabs(zTarValue) > y){
      						// Scale Bokeh
                            int extendBelow = 0;
                            float distanceToPixel = length(float2(x,y));
                            float saveZTarValue = zTarValue;


                            if (depth(0,0,0) -  s > -1 && depth(0,0,0)  -  s <= 0 && zTarValue -  s < -1 && src(x,y,3) > 0) {
                                zTarValue = zTarValue+distanceToPixel;
                                extendBelow = 1;
                            }

      						xyBokeh.x = (((float)(x) / CurSlice *-1) / (fabs(zTarValue)/CurSlice) +1) *  (bokehOffset.x);
      						xyBokeh.y = (((float)(y) / CurSlice *-1) / (fabs(zTarValue)/CurSlice) +1) *  (bokehOffset.y);

      						//Clamp max Bokeh position value
      						xyBokeh.x = clamp(xyBokeh.x, 0.0f,(float)(bokeh.bounds.width()));
      						xyBokeh.y = clamp(xyBokeh.y, 0.0f,(float)(bokeh.bounds.height()));

      						bokeh_Color = bilinear(bokeh, xyBokeh.x, xyBokeh.y);

      						bokeh_val = (bokeh_Color.x*0.299   + bokeh_Color.y *0.587 + bokeh_Color.z*0.114   	);
                            zTarValue = saveZTarValue;
      						if(bokeh_val > 0) {
                                if(zTarValue -  s > -1 && zTarValue -  s <= 0 || extendBelow){


      								Cat.x = ((float)(x) / CurSlice)/ (fabs(zTarValue)/CurSlice) + (direction.x * strength * CatEye_Strength* CatEye_Size *  CornerDist );
      								Cat.y = ((float)(y) / CurSlice)/ (fabs(zTarValue)/CurSlice) + (direction.y * strength * CatEye_Strength* CatEye_Size *  CornerDist );

      								if(length(Cat) < CatEye_Size){
                                        // if(zTarValue -  s <= 0) {


    										valueSum += bokeh_Color * src(x,y);
      										bokehSum += bokeh_val;
                                            // valueSum += src(0,0);
      										// bokehSum += 1;
      										alphaHit += 1;
      									// }
                                    }

                                }
                            }
                            if ( zTarValue -  s <= -1 && src(x,y,3) == 0.0f) {
                              alphaNoHit += 1;
                            }

      								// else{
      								// 	alphaNoHit += 1;
      								// }
      							// else{
      							// 	alphaNoHit += 1;
      							// }

      					// } // if (fabs(zTarValue) > x || fabs(zTarValue) > y){

                    }
				}
			}
		}


		if(inFocus == 1){
			setColor = 1;

			alpha = 1.0f;
			RGB = src(0,0);
		}
		else{

			if (bokehSum != 0){
				setColor = 1;

				alpha = 1  - (alphaNoHit   /(alphaHit + alphaNoHit));
				RGB = valueSum / bokehSum ;
			}
		}

		if(setColor ==  1){

			//premultiplied
			combine.w = alpha +  combine.w *(1.0 - alpha);
			combine.x = (RGB.x * alpha +  combine.x  * (1.0f-alpha));
			combine.y = (RGB.y * alpha +  combine.y  * (1.0f-alpha));
			combine.z = (RGB.z * alpha +  combine.z  * (1.0f-alpha));


			//unpremultiplied
			//combine.w = alpha + 1 *(1.0 - alpha);
			//combine.x = (RGB.x * alpha +  combine.x * combine.w*(1.0f-alpha)) / combine.w;
			//combine.y = (RGB.y * alpha +  combine.y * combine.w*(1.0f-alpha)) / combine.w;
			//combine.z = (RGB.z * alpha +  combine.z * combine.w*(1.0f-alpha)) / combine.w;
		}
	}

	return combine	;
  }
};