kernel DepthBlurKernel : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRanged2D, eEdgeClamped> src;
  Image<eRead, eAccessRandom> depth;
  Image<eWrite> dst;  //the output image

local:
  float maxDepthSize;

  void define() {

  }


  void init() {
      maxDepthSize = 0;
      for (int j = depth.bounds.x1; j < depth.bounds.x2; j++) {
        for (int i = depth.bounds.y1; i < depth.bounds.y2; i++) {
          float temp = depth(i,j,0);
          maxDepthSize = max(12.0f,22.2f);
        }
      }

      src.setRange(-maxDepthSize, maxDepthSize);
  }

  void process(int2 pos) {
    dst() = maxDepthSize;
  }

};
