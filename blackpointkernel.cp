
kernel BlackPointKernel : ImageComputationKernel<ePixelWise> {

    Image<eRead, eAccessRanged2D, eEdgeClamped> src;
    Image<eWrite, eAccessRandom, eEdgeClamped> dst;

    param:

    float4 mincolor_input;
    float4 maxcolor_input;

    int dividerx;
    int dividery;


    local:

    // float4 mincolor;
    // float4 maxcolor;

    int2 mod;
    int2 modrange;

    float minColorBrightness;
    float maxColorBrightness;



    // define
    void define() {

        defineParam(mincolor_input, "Min Color", float4(0));
        defineParam(maxcolor_input, "Max Color", float4(100.0f));
        defineParam(dividerx, "Divider x", 3);
        defineParam(dividery, "Divider y", 3);

    }


    // init
    void init() {

        // mincolor = mincolor_input;
        // maxcolor = maxcolor_input;

        minColorBrightness = mincolor_input.x*0.299f + mincolor_input.y*0.587f + mincolor_input.z*0.114f;
        maxColorBrightness = maxcolor_input.x*0.299f + maxcolor_input.y*0.587f + maxcolor_input.z*0.114f;

        int cdividerx;
        int cdividery;
        if (dividerx < 1) {cdividerx = 1;} else {cdividerx=dividerx;}
        if (dividery < 1) {cdividery = 1;} else {cdividery=dividery;}

        mod.x = (int)floor((float)src.bounds.x2 / (float)cdividerx);
        mod.y = (int)floor((float)src.bounds.y2 / (float)cdividery);
        if (cdividerx%2==0) {mod.x -= 1;}
        if (cdividery%2==0) {mod.y -= 1;}

        modrange.x = (int)ceil(mod.x/2);
        modrange.y = (int)ceil(mod.y/2);

        src.setRange(-modrange.x, -modrange.y, modrange.x, modrange.y);
    }


    // process
    void process(int2 pos) {

        // center pattern
        int2 poscentered;
        poscentered.x = (int)floor((float)pos.x-(float)dst.bounds.x2/2.0);
        poscentered.y = (int)floor((float)pos.y-(float)dst.bounds.y2/2.0);

        // only execute on pixels that are necessary
        if (poscentered.x%mod.x == 0 && poscentered.y%mod.y == 0) {

            SampleType(src) cell_mincolor(maxcolor_input);
            float cell_mincolor_brightness = maxColorBrightness;

            // find min brightness in modrange
            for (int i = -modrange.x; i <= modrange.x; i++) {
                for (int j = -modrange.y; j <= modrange.y; j++) {

                    if (pos.x+i >= dst.bounds.x1 && pos.x+i <= dst.bounds.x2 && pos.y+j >= dst.bounds.y1 && pos.y+j <= dst.bounds.y2) {
                        SampleType(src) cursample = src(i,j);
                        float sampleBrightness = cursample.x*0.299f + cursample.y*0.587f + cursample.z*0.114f;

                        if (sampleBrightness < cell_mincolor_brightness && sampleBrightness >= minColorBrightness) {
                            cell_mincolor = cursample;
                            cell_mincolor_brightness = sampleBrightness;
                        }
                    }

                }
            }

            // write pixels in modrange
            for (int i = -modrange.x; i <= modrange.x; i++) {
                for (int j = -modrange.y; j <= modrange.y; j++) {

                    if (pos.x+i >= dst.bounds.x1 && pos.x+i <= dst.bounds.x2 && pos.y+j >= dst.bounds.y1 && pos.y+j <= dst.bounds.y2) {
                        dst(pos.x+i,pos.y+j) = cell_mincolor;
                    }

                }
            }


        }
    }
};
