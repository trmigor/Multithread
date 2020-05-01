#include "Matrix.hpp"

#include <iostream>
#include <thread>

#define forn(i, n) for (size_t i = 0; i < size_t(n); ++i)
#define forf(i, n1, n2) for (size_t i = size_t(n1); i < size_t(n2); ++i)

namespace matrix {

double operator*(const Vector& left, const Vector& right) {
    if (left.size() != right.size()) {
        throw "Vector: operator*: unappropriate arguments";
    }

    double res = 0.0;
    forn(i, left.size()) {
        res += left[i] * right[i];
    }
    return res;
}

std::ostream& operator<<(std::ostream& os, const Vector& to_print) {
    os << "[ ";
    forn(i, to_print.size()) {
        os << to_print[i] << " ";
    }
    os << "]";
    return os;
}

Matrix::Matrix(size_t rows, size_t cols, size_t num_threads) :
    val_(__matrix(rows, Vector(cols, 0))),
    rows_(rows), cols_(cols), nThreads_(num_threads) {}

Matrix::Matrix(const __matrix& val) : val_(val), rows_(val.size()) {
    if (rows_ != 0) {
        cols_ = val[0].size();
    } else {
        cols_ = 0;
    }
}

Matrix::Matrix(const Matrix& other) : val_(other.val_), rows_(other.rows_),
    cols_(other.cols_) {}

Matrix::~Matrix() {}

Matrix Matrix::operator=(const Matrix& other) {
    this->val_ = other.val_;
    this->rows_ = other.rows_;
    this->cols_ = other.cols_;
    this->nThreads_ = other.nThreads_;
    return *this;
}

__m_size_t Matrix::Size() const { return __m_size_t(rows_, cols_); }
size_t Matrix::Rows() const { return rows_; }
size_t Matrix::Cols() const { return cols_; }

Vector& Matrix::operator[](size_t i) { return val_[i]; }
Vector Matrix::operator[](size_t i) const { return val_[i]; }

Matrix Matrix::computeTransposed() const {
    __matrix res(cols_, Vector(rows_, 0));
    forn(i, rows_) {
        forn(j, cols_) {
            res[j][i] = val_[i][j];
        }
    }
    return Matrix(res);
}

void threadMultiplyJob(Matrix::__thr_m_j_input in) {
    forf(i, in.lefti, in.righti) {
        forn(j, in.tright.Rows()) {
            in.res[i][j] = in.left[i] * in.tright[j];
        }
    }
}

Matrix operator*(const Matrix& left, const Matrix& right) {
    __matrix res(left.rows_, Vector(right.cols_, 0));
    const Matrix tright = right.computeTransposed();
    std::vector<std::thread> threads;
    forn(i, left.nThreads_) {
        size_t lefti = i * (res.size() / left.nThreads_);
        size_t righti = i == left.nThreads_ - 1 ?
                res.size() : (i + 1) * (res.size() / left.nThreads_);
        Matrix::__thr_m_j_input in = {left, tright, lefti, righti, res};
        threads.push_back(std::thread(&threadMultiplyJob, in));
    }

    forn(i, left.nThreads_) {
        threads[i].join();
    }
    return Matrix(res);
}

std::ostream& operator<<(std::ostream& os, Matrix to_print) {
    os << "[" << std::endl;
    forn(i, to_print.Rows()) {
        os << to_print[i] << std::endl;
    }
    os << "]";
    return os;
}

}  // namespace matrix

#undef forn
#undef forf
