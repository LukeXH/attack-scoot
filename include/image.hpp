#ifndef IMAGE_H
#define IMAGE_H
/**
 * Class to store and manipulate an Image8b, cribbed from openCV
 */

 #include <cassert>
 #include <iostream>
 #include <stdexcept>

typedef unsigned char uchar;

// From opencv2/core/hal/interface.h
#define CN_MAX     512 //CV_CN_MAX
#define CN_SHIFT   3 // CV_CN_SHIFT
#define DEPTH_MAX  (1 << CN_SHIFT) // CV_DEPTH_MAX
#define MAT_DEPTH_MASK       (DEPTH_MAX - 1) //CV_MAT_DEPTH_MASK
#define MAT_DEPTH(flags)     ((flags) & MAT_DEPTH_MASK) // CV_MAT_DEPTH

// from opencv2/core/cvdef.h
#define MAT_CN_MASK          ((CN_MAX - 1) << CN_SHIFT)
#define MAT_CN(flags)        ((((flags) & MAT_CN_MASK) >> CN_SHIFT) + 1)
// Data type defines
#define IM_8U   0 // CV_8U
#define MAKETYPE(depth,cn) (MAT_DEPTH(depth) + (((cn)-1) << CN_SHIFT)) // CV_MAKETYPE
#define IM_8UC1 MAKETYPE(IM_8U,1) // CV_8UC1
#define IM_8UC3 MAKETYPE(IM_8U,3) // CV_8UC3
#define IM_8UC4 MAKETYPE(IM_8U,4)

///////////////////////////// ImStep ////////////////////////////
struct ImStep
{
    ImStep();
    explicit ImStep(size_t s);
    const size_t& operator[](int i) const;
    size_t& operator[](int i);
    operator size_t() const;
    ImStep& operator = (size_t s);

    size_t* p;
    size_t buf[2];
};

inline
ImStep::ImStep()
{
    p = buf; p[0] = p[1] = 0;
}

inline
ImStep::ImStep(size_t s)
{
    p = buf; p[0] = s; p[1] = 0;
}

inline
const size_t& ImStep::operator[](int i) const
{
    return p[i];
}

inline
size_t& ImStep::operator[](int i)
{
    return p[i];
}

inline ImStep::operator size_t() const
{
    assert( p == buf );
    return buf[0];
}

inline ImStep& ImStep::operator = (size_t s)
{
    assert( p == buf );
    buf[0] = s;
    return *this;
}

class Image8b
{
    /**
     * Storage wrapper for 8bit image with 3 channels
     */
private:
    /* data */
public:
    Image8b();
    Image8b(int _rows, int _cols, int _type, void* _data, size_t _step=AUTO_STEP);
    ~Image8b() = default;

    /** @overload
    @param row Index along the dimension 0
    @param col Index along the dimension 1
    */
//    template<typename _Tp> _Tp& at(int row, int col);
    uchar& at(int row, int col);
    // uchar& at(int row, int col, int channel);

    enum { MAGIC_VAL  = 0x42FF0000, AUTO_STEP = 0}; //, CONTINUOUS_FLAG = CV_MAT_CONT_FLAG, SUBMATRIX_FLAG = CV_SUBMAT_FLAG };
    enum { MAGIC_MASK = 0xFFFF0000, TYPE_MASK = 0x00000FFF, DEPTH_MASK = 7 };

    // Image type, encoded in integer, not quick sure how flags work yet
    int im_type;
    //! the matrix dimensionality, >= 2
    int dims;
    //! the number of rows and columns or (-1, -1) when the matrix has more than 2 dimensions
    int rows, cols;
    //! pointer to the data
    uchar* data;

    //! helper fields used in locateROI and adjustROI
    const uchar* datastart;
    const uchar* dataend;
    const uchar* datalimit;

    ImStep step;
};

Image8b::Image8b()
    : im_type(0), dims(0), rows(-1), cols(-1)
{}

Image8b::Image8b(int _rows, int _cols, int _type, void* _data, size_t _step)
    : im_type(_type), dims(2), rows(_rows), cols(_cols), data((uchar*)_data), 
    datastart((uchar*)_data), dataend(0), datalimit(0)
{
    assert(data != NULL);

    /** Size of each channel item,
    0x28442211 = 0010 1000 0100 0100 0010 0010 0001 0001 ~ array of sizeof(arr_type_elem) */
    size_t esz1 = (0x28442211 >> ((_type) & MAT_DEPTH_MASK)*4) & 15;
    size_t esz = MAT_CN(_type) * esz1;
    size_t minstep = cols * esz;
    if( _step == AUTO_STEP )
    {
        _step = minstep;
    }
    else
    {
        assert( _step >= minstep);
        if( _step % esz1 != 0)
        {
            std::cout << "esz1 is: " << esz1 << std::endl;
            throw std::runtime_error("ERROR::BadStep: Step must be a multiple of esz1");            
        }
    }
    step[0] = _step;
    step[1] = esz;
    datalimit = datastart + _step * rows;
    dataend = datalimit - _step + minstep;
}

inline
uchar& Image8b::at(int row, int col)
{
    assert(dims <= 2);
    assert(data);
    // assert((unsigned)row < (unsigned)size.p[0]); // TODO Implement this check
    // assert((unsigned)(col * MAT_CN(im_type)) < (unsigned)(size.p[1] * MAT_CN(im_type))); // TODO Implement this check
    // assert(CV_ELEM_SIZE1(traits::Depth<_Tp>::value) == elemSize1()); // TODO Implement this check
    return ((uchar*)(data + step.p[0] * row))[col];
}

// inline
// uchar& Image8b::at(int row, int col, int channel)
// {
//     assert(channel < MAT_CN(im_type));
//     return at(row,col)[channel];
// }

#endif /* IMAGE_H */