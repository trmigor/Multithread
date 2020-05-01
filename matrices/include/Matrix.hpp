#include <vector>
#include <utility>

#ifndef MATRICES_INCLUDE_MATRIX_HPP_
#define MATRICES_INCLUDE_MATRIX_HPP_

namespace matrix {

typedef std::vector<double> Vector;
typedef std::vector<Vector> __matrix;
typedef std::pair<size_t, size_t> __m_size_t;

class Matrix {
    public:
        struct __thr_m_j_input {
            const Matrix& left;
            const Matrix& tright;
            const size_t lefti;
            const size_t righti;
            __matrix& res;
        };

        Matrix(void) = delete;

        explicit Matrix(size_t rows, size_t cols = 0, size_t num_threads = 1);

        explicit Matrix(const __matrix& val);

        Matrix(const Matrix& other);

        Matrix operator=(const Matrix& other);

        __m_size_t Size() const;
        size_t Rows() const;
        size_t Cols() const;

        Vector& operator[](size_t i);
        Vector operator[](size_t i) const;

        Matrix computeTransposed() const;

        friend Matrix operator*(const Matrix& left, const Matrix& right);

        ~Matrix();

    private:
        __matrix val_;
        size_t rows_;
        size_t cols_;

        size_t nThreads_;
};

void threadMultiplyJob(Matrix::__thr_m_j_input in);

std::ostream& operator<<(std::ostream& os, Matrix to_print);

}  // namespace matrix

#endif  // MATRICES_INCLUDE_MATRIX_HPP_
