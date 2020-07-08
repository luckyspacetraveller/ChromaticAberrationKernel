kernel RotateImage123 : ImageComputationKernel<ePixelWise>
{
  Image<eRead, eAccessRandom, eEdgeConstant> src;
  Image<eWrite, eAccessPoint> dst;

param:
  float2 _center;
//  float2 _size;
  float _rotate;



  void define() {
    defineParam(_center, "center", float2(960.0f, 540.0f));
    defineParam(_rotate, "rotate", float(90.0f));
  }

  void init() {

  }

  void process(int2 pos) {

      float x = (float)pos.x;
      float y = (float)pos.y;

      float dx = (x - _center.x);
      float dy = (y - _center.y);

      float cs = cos( _rotate * 3.1415926535f/180.0f);
      float sn = sin( _rotate * 3.1415926535f/180.0f);



      x = dx*cs + dy * sn  + _center.x;
      y = -dx*sn + dy * cs  + _center.y;


    dst() = bilinear(src,x,y);

  }
};
