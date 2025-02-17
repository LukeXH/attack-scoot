/**
 * Class to store and manipulate an cvMat, cribbed from openCV
 * Mostly going to be using this as a template for a more stripped
 * down version that we're going to call "image".
 */

#include <cassert>
#include <iostream>
#include <stdexcept>

 #ifndef CV_NOEXCEPT
 #  define CV_NOEXCEPT noexcept
 #endif

typedef unsigned char uchar;

/****************************************************************************************\
*                                  Matrix type (Mat)                                     *
\****************************************************************************************/

// From opencv2/core/hal/interface.h
#define CV_CN_MAX     512
#define CV_CN_SHIFT   3
#define CV_DEPTH_MAX  (1 << CV_CN_SHIFT)
#define CV_MAT_DEPTH_MASK       (CV_DEPTH_MAX - 1)
#define CV_MAT_DEPTH(flags)     ((flags) & CV_MAT_DEPTH_MASK)

#define CV_8U   0
#define CV_MAKETYPE(depth,cn) (CV_MAT_DEPTH(depth) + (((cn)-1) << CV_CN_SHIFT))
#define CV_MAKE_TYPE CV_MAKETYPE
#define CV_8UC1 CV_MAKETYPE(CV_8U,1)

// from opencv2/core/cvdef.h
#define CV_MAX_DIM              32
#define CV_MAT_CN_MASK          ((CV_CN_MAX - 1) << CV_CN_SHIFT)
#define CV_MAT_CN(flags)        ((((flags) & CV_MAT_CN_MASK) >> CV_CN_SHIFT) + 1)
#define CV_MAT_TYPE_MASK        (CV_DEPTH_MAX*CV_CN_MAX - 1)
#define CV_MAT_TYPE(flags)      ((flags) & CV_MAT_TYPE_MASK)
#define CV_MAT_CONT_FLAG_SHIFT  14
#define CV_MAT_CONT_FLAG        (1 << CV_MAT_CONT_FLAG_SHIFT)
#define CV_IS_MAT_CONT(flags)   ((flags) & CV_MAT_CONT_FLAG)
#define CV_IS_CONT_MAT          CV_IS_MAT_CONT
#define CV_SUBMAT_FLAG_SHIFT    15
#define CV_SUBMAT_FLAG          (1 << CV_SUBMAT_FLAG_SHIFT)
#define CV_IS_SUBMAT(flags)     ((flags) & CV_MAT_SUBMAT_FLAG)

/** Size of each channel item,
   0x28442211 = 0010 1000 0100 0100 0010 0010 0001 0001 ~ array of sizeof(arr_type_elem) */
#define CV_ELEM_SIZE1(type) ((0x28442211 >> CV_MAT_DEPTH(type)*4) & 15)

#define CV_ELEM_SIZE(type) (CV_MAT_CN(type)*CV_ELEM_SIZE1(type))

///////////////////////////// MatStep ////////////////////////////
struct MatStep
{
    MatStep() CV_NOEXCEPT;
    explicit MatStep(size_t s) CV_NOEXCEPT;
    const size_t& operator[](int i) const CV_NOEXCEPT;
    size_t& operator[](int i) CV_NOEXCEPT;
    operator size_t() const;
    MatStep& operator = (size_t s);

    size_t* p;
    size_t buf[2];
protected:
    MatStep& operator = (const MatStep&);
};

inline
MatStep::MatStep() CV_NOEXCEPT
{
    p = buf; p[0] = p[1] = 0;
}

inline
MatStep::MatStep(size_t s) CV_NOEXCEPT
{
    p = buf; p[0] = s; p[1] = 0;
}

inline
const size_t& MatStep::operator[](int i) const CV_NOEXCEPT
{
    return p[i];
}

inline
size_t& MatStep::operator[](int i) CV_NOEXCEPT
{
    return p[i];
}

inline MatStep::operator size_t() const
{
    assert( p == buf );
    return buf[0];
}

inline MatStep& MatStep::operator = (size_t s)
{
    assert( p == buf );
    buf[0] = s;
    return *this;
}

// struct MatSize
// {
//     explicit MatSize(int* _p) CV_NOEXCEPT;
//     int dims() const CV_NOEXCEPT;
//     // Size operator()() const;
//     const int& operator[](int i) const;
//     int& operator[](int i);
//     operator const int*() const CV_NOEXCEPT;  // TODO OpenCV 4.0: drop this
//     bool operator == (const MatSize& sz) const CV_NOEXCEPT;
//     bool operator != (const MatSize& sz) const CV_NOEXCEPT;

//     int* p;
// };


///////////////////////////// cvMat ////////////////////////////
class cvMat
{
private:
    /* data */
public:
    cvMat() CV_NOEXCEPT;
    cvMat(int _rows, int _cols, int _type, void* _data, size_t _step=AUTO_STEP);
    ~cvMat();

    int channels() const;

    /** @overload
    @param row Index along the dimension 0
    @param col Index along the dimension 1
    */
    template<typename _Tp> _Tp& at(int row, int col);

    // size_t total() const;

    enum { MAGIC_VAL  = 0x42FF0000, AUTO_STEP = 0}; //, CONTINUOUS_FLAG = CV_MAT_CONT_FLAG, SUBMATRIX_FLAG = CV_SUBMAT_FLAG };
    enum { MAGIC_MASK = 0xFFFF0000, TYPE_MASK = 0x00000FFF, DEPTH_MASK = 7 };

    int flags;
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

    //! custom allocator
    // MatAllocator* allocator; //# To be implemented later

    //! interaction with UMat
    // UMatData* u;

    // MatSize size; //# To be implemented later
    MatStep step;
};


cvMat::cvMat(int _rows, int _cols, int _type, void* _data, size_t _step)
    : flags(MAGIC_VAL + (_type & TYPE_MASK)), dims(2), rows(_rows), cols(_cols),
    data((uchar*)_data), datastart((uchar*)_data), dataend(0), datalimit(0)//,
    //size(&rows) , allocator(0), u(0)
{
    assert(data != NULL); // CV_Assert(total() == 0 || data != NULL);
    
    size_t esz = CV_ELEM_SIZE(_type), esz1 = CV_ELEM_SIZE1(_type);
    size_t minstep = cols * esz;
    if( _step == AUTO_STEP )
    {
        _step = minstep;
    }
    else
    {
        assert( _step >= minstep );
        if (_step % esz1 != 0)
        {
            throw std::runtime_error("ERROR::BadStep: Step must be a multiple of esz1");            
        }
    }
    step[0] = _step;
    step[1] = esz;
    datalimit = datastart + _step * rows;
    dataend = datalimit - _step + minstep;
}

cvMat::~cvMat()
{
}

///////////////////////// cvMat Inline funcs ////////////////////////////
inline
int cvMat::channels() const
{
    return CV_MAT_CN(flags);
}

template<typename _Tp> inline
_Tp& cvMat::at(int i0, int i1)
{
    assert(dims <= 2);
    assert(data);
    assert((unsigned)i0 < (unsigned)size.p[0]);
    assert((unsigned)(i1 * DataType<_Tp>::channels) < (unsigned)(size.p[1] * channels()));
    assert(CV_ELEM_SIZE1(traits::Depth<_Tp>::value) == elemSize1());
    return ((_Tp*)(data + step.p[0] * i0))[i1];
}

// size_t cvMat::total() const
// {
//     if( dims <= 2 )
//         return (size_t)rows * cols;
//     size_t p = 1;
//     for( int i = 0; i < dims; i++ )
//         p *= size[i];
//     return p;
// }