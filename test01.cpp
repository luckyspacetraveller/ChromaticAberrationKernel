//Warning: connecting a large image to the filter input will cause the kernel to run very slowly!
//If running on a GPU connected to a display, this will cause problems if the time taken to
//execute the kernel is longer than your operating system allows. Use with caution!
kernel ConvolutionKernel : public ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRanged2D, eEdgeClamped> src;
  Image<eRead, eAccessRandom, eEdgeClamped> depth;
  Image<eRead, eAccessRandom> filter;
  Image<eWrite> dst;

param:
  float2 center;

local:
  int2 _sizeInv;
  int2 _filterOffset;

  void define() {
    defineParam(center, "center", float2(960.0f, 540.0f));
  }

  void init() {
      //Get the size of the filter input and store the radius.
    int2 filterRadius(filter.bounds.width()/2, filter.bounds.height()/2);

    //Store the offset of the bottom-left corner of the filter image
    //from the current pixel:
    _filterOffset[0] = filter.bounds.x1 - filterRadius[0];
    _filterOffset[1] = filter.bounds.y1 - filterRadius[1];

    //Set up the access for the src image
    src.setRange(-filterRadius[0], -filterRadius[1], filterRadius[0], filterRadius[1]);
  }
};
