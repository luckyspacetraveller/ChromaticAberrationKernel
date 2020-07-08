kernel DepthBlurKernel : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRanged2D, eEdgeClamped> src;
  Image<eRead, eAccessRandom> bokeh;
  Image<eRead, eAccessRandom> depth;
  Image<eWrite> dst;  //the output image

param:
  float2 focus;

local:
  float maxDepthSize;
  int2 _bokehOffset;

  //In define(), parameters can be given labels and default values.
  void define() {
  }

  //The init() function is run before any calls to process().
  void init() {
      int2 bokehRadius(bokeh.bounds.width()/2, bokeh.bounds.height()/2);

//Store the offset of the bottom-left corner of the bokeh image
//from the current pixel:
      _bokehOffset[0] = bokeh.bounds.x1 - bokehRadius[0];
      _bokehOffset[1] = bokeh.bounds.y1 - bokehRadius[1];
      maxDepthSize = 15.0f;
      src.setRange(-maxDepthSize, maxDepthSize);
  }

  //The process function is run at every pixel to produce the output.
  void process(int2 pos) {
    //Get the depth at the current pixel
        SampleType(src) valueSum(0);
        ValueType(bokeh) bokehSum(0);
        float z;

        //Iterate over the bokeh image
        for(int j = -maxDepthSize; j < maxDepthSize; j++) {
          for(int i = -maxDepthSize; i < maxDepthSize; i++) {

           z = depth(pos.x+j,pos.y+i,0);

            if (z > j && z > i && z > 1 ) {

                float2 bokehPos = (i + _bokehOffset[0]*(i/z),j + _bokehOffset[1]*(j/z));

                ValueType(bokeh) bokehVal = bilinear(bokeh, bokehPos.x,  bokehPos.y, 0);


                valueSum += bokehVal * src(i + _bokehOffset[0], j + _bokehOffset[1]);


                bokehSum += bokehVal;

            }
            else {
                valueSum += src(i + _bokehOffset[0], j + _bokehOffset[1]);
            }
          }
        }

        //Normalise the value sum, avoiding division by zero
        if (bokehSum != 0)
          valueSum /= bokehSum;

    dst() = valueSum;

  }
};
