#ifndef OPTICAL_FLOW_H
#define OPTICAL_FLOW_H

#include "include/image.hpp"

namespace oflow 
{

inline
uchar charmax(uchar a, uchar b)
{
    return (a>b) ? a : b;
}

uchar getLightness(uchar* im_addr)
{
    return charmax(im_addr[0], charmax(im_addr[1], im_addr[2]));
}

void calcOFlow(Image8b& image0, Image8b& image1, int steps = 1)
{
/**
 * Calculate optical flow in im1 (im0 is from the frame before)
 */
    assert(steps > 0);
    assert( (image0.rows*image0.cols*MAT_CN(image0.im_type)) % steps == 0);
    assert( (image1.rows*image1.cols*MAT_CN(image1.im_type)) % steps == 0);

    for(uchar *im0 = (uchar*)image0.datastart, *im1 = (uchar*)image1.datastart;
            im0 <= image0.dataend && im1 <= image1.dataend;
            std::advance(im0,steps*image0.step.p[1]), std::advance(im1,steps*image1.step.p[1]))
    {
        // Calculate intensities
        uchar im0_I = charmax(im0[0], charmax(im0[1], im0[2])); // Method for getting intensity
        uchar im1_I = charmax(im1[0], charmax(im1[1], im1[2])); // Method for getting intensity

        // Calculate time derivative
        int16_t I_dt = (int16_t)im1_I - (int16_t)im0_I;

        // // Calculate col derivative (x)
        uchar *im1_xn = image1.getneighbor(im1, -1, 0); // "left" in row
        uchar *im1_xp = image1.getneighbor(im1, 1, 0); // "right" in row
        uchar *im1_yn = image1.getneighbor(im1, 0, -1); // "above" in row
        uchar *im1_yp = image1.getneighbor(im1, 0, 1); // "below" in row
        // int16_t
    }
}

} /* End namespace oflow */
#endif /* OPTICAL_FLOW_H */