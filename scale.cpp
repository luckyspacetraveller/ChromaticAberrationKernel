kernel RotateImage123 : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRanged1D, eEdgeConstant> src;
  Image<eWrite, eAccessPoint> dst;

param:

  float3 shift;

  void define() {
    defineParam(shift, "shift", float3(0));

  }

  void init() {
      src.setAxis(eX);

      src.setRange(-src.bounds.x1,src.bounds.x1);

  }

  void process() {

    dst(0) = src((int)shift.x,0);
    dst(1) = src((int)shift.y,1);
    dst(2) = src((int)shift.z,2);

  }
};
