#pragma once

// Eigen stub header for development without Eigen
// This provides basic Eigen types for compilation

#include <vector>
#include <cstddef>

namespace Eigen {

// Basic types
using Index = std::ptrdiff_t;

// Matrix class stub
template<typename Scalar>
class Matrix {
public:
    Matrix() : rows_(0), cols_(0) {}
    Matrix(Index rows, Index cols) : rows_(rows), cols_(cols), data_(rows * cols) {}
    
    Index rows() const { return rows_; }
    Index cols() const { return cols_; }
    Index size() const { return data_.size(); }
    
    Scalar& operator()(Index i, Index j) { return data_[i * cols_ + j]; }
    const Scalar& operator()(Index i, Index j) const { return data_[i * cols_ + j]; }
    
    Scalar& operator[](Index i) { return data_[i]; }
    const Scalar& operator[](Index i) const { return data_[i]; }
    
    // Data access for serialization
    const Scalar* data() const { return data_.data(); }
    Scalar* data() { return data_.data(); }
    
    void resize(Index rows, Index cols) {
        rows_ = rows;
        cols_ = cols;
        data_.resize(rows * cols);
    }
    
    void setZero() {
        std::fill(data_.begin(), data_.end(), Scalar(0));
    }
    
    void setOnes() {
        std::fill(data_.begin(), data_.end(), Scalar(1));
    }
    
    void setRandom() {
        // Simple random initialization
        for (auto& val : data_) {
            val = Scalar(rand()) / Scalar(RAND_MAX);
        }
    }
    
    Matrix operator+(const Matrix& other) const {
        Matrix result(rows_, cols_);
        for (Index i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] + other.data_[i];
        }
        return result;
    }
    
    Matrix operator-(const Matrix& other) const {
        Matrix result(rows_, cols_);
        for (Index i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] - other.data_[i];
        }
        return result;
    }
    
    Matrix operator*(const Matrix& other) const {
        Matrix result(rows_, other.cols_);
        for (Index i = 0; i < rows_; ++i) {
            for (Index j = 0; j < other.cols_; ++j) {
                Scalar sum = 0;
                for (Index k = 0; k < cols_; ++k) {
                    sum += (*this)(i, k) * other(k, j);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }
    
    Matrix operator*(Scalar scalar) const {
        Matrix result(rows_, cols_);
        for (Index i = 0; i < data_.size(); ++i) {
            result.data_[i] = data_[i] * scalar;
        }
        return result;
    }
    
    Matrix& operator+=(const Matrix& other) {
        for (Index i = 0; i < data_.size(); ++i) {
            data_[i] += other.data_[i];
        }
        return *this;
    }
    
    Matrix& operator-=(const Matrix& other) {
        for (Index i = 0; i < data_.size(); ++i) {
            data_[i] -= other.data_[i];
        }
        return *this;
    }
    
    Matrix& operator*=(Scalar scalar) {
        for (Index i = 0; i < data_.size(); ++i) {
            data_[i] *= scalar;
        }
        return *this;
    }
    
    Matrix& operator/=(Scalar scalar) {
        if (scalar != 0) {
            for (Index i = 0; i < data_.size(); ++i) {
                data_[i] /= scalar;
            }
        }
        return *this;
    }
    
    Scalar norm() const {
        Scalar sum = 0;
        for (const auto& val : data_) {
            sum += val * val;
        }
        return std::sqrt(sum);
    }
    
    Scalar squaredNorm() const {
        Scalar sum = 0;
        for (const auto& val : data_) {
            sum += val * val;
        }
        return sum;
    }
    
    void fill(Scalar value) {
        std::fill(data_.begin(), data_.end(), value);
    }
    
    // Transpose
    Matrix transpose() const {
        Matrix result(cols_, rows_);
        for (Index i = 0; i < rows_; ++i) {
            for (Index j = 0; j < cols_; ++j) {
                result(j, i) = (*this)(i, j);
            }
        }
        return result;
    }
    
    // Block operation
    Matrix block(Index startRow, Index startCol, Index blockRows, Index blockCols) const {
        Matrix result(blockRows, blockCols);
        for (Index i = 0; i < blockRows; ++i) {
            for (Index j = 0; j < blockCols; ++j) {
                result(i, j) = (*this)(startRow + i, startCol + j);
            }
        }
        return result;
    }
    
    // Block operation (mutable)
    Matrix& block(Index startRow, Index startCol, Index blockRows, Index blockCols) {
        // This is a simplified version - in real Eigen this would return a view
        static Matrix temp;
        temp.resize(blockRows, blockCols);
        for (Index i = 0; i < blockRows; ++i) {
            for (Index j = 0; j < blockCols; ++j) {
                temp(i, j) = (*this)(startRow + i, startCol + j);
            }
        }
        return temp;
    }
    
    // Identity matrix
    static Matrix Identity(Index size) {
        Matrix result(size, size);
        result.setZero();
        for (Index i = 0; i < size; ++i) {
            result(i, i) = Scalar(1);
        }
        return result;
    }
    
    // Zero matrix
    static Matrix Zero(Index rows, Index cols) {
        Matrix result(rows, cols);
        result.setZero();
        return result;
    }
    
    // Ones matrix
    static Matrix Ones(Index rows, Index cols) {
        Matrix result(rows, cols);
        result.setOnes();
        return result;
    }
    
    // Random matrix
    static Matrix Random(Index rows, Index cols) {
        Matrix result(rows, cols);
        result.setRandom();
        return result;
    }
    
    // Row and column access
    // Row operations
    Matrix row(Index i) const {
        Matrix result(1, cols_);
        for (Index j = 0; j < cols_; ++j) {
            result(0, j) = (*this)(i, j);
        }
        return result;
    }
    
    void row(Index i, const Matrix& new_row) {
        if (new_row.rows() == 1 && new_row.cols() == cols_) {
            for (Index j = 0; j < cols_; ++j) {
                (*this)(i, j) = new_row(0, j);
            }
        }
    }
    
    Matrix col(Index j) const {
        Matrix result(rows_, 1);
        for (Index i = 0; i < rows_; ++i) {
            result(i, 0) = (*this)(i, j);
        }
        return result;
    }
    
    // Array operations
    class ArrayWrapper {
    private:
        const Matrix& matrix_;
    public:
        ArrayWrapper(const Matrix& matrix) : matrix_(matrix) {}
        
        Matrix square() const {
            Matrix result = matrix_;
            for (Index i = 0; i < result.size(); ++i) {
                result.data()[i] = result.data()[i] * result.data()[i];
            }
            return result;
        }
    };
    
    ArrayWrapper array() const {
        return ArrayWrapper(*this);
    }

private:
    Index rows_;
    Index cols_;
    std::vector<Scalar> data_;
};

// Vector class (special case of Matrix)
template<typename Scalar>
class Vector : public Matrix<Scalar> {
public:
    Vector() : Matrix<Scalar>(0, 1) {}
    Vector(Index size) : Matrix<Scalar>(size, 1) {}
    
    Index size() const { return this->rows(); }
    
    Scalar& operator[](Index i) { return (*this)(i, 0); }
    const Scalar& operator[](Index i) const { return (*this)(i, 0); }
    
    void resize(Index size) {
        Matrix<Scalar>::resize(size, 1);
    }
    
    // Vector-specific operations
    Scalar dot(const Vector& other) const {
        Scalar result = 0;
        for (Index i = 0; i < size(); ++i) {
            result += (*this)[i] * other[i];
        }
        return result;
    }
    
    // Additional vector operations
    Scalar sum() const {
        Scalar result = 0;
        for (Index i = 0; i < size(); ++i) {
            result += (*this)[i];
        }
        return result;
    }
    
    Scalar mean() const {
        return sum() / size();
    }
    
    Scalar minCoeff() const {
        if (size() == 0) return 0;
        Scalar min_val = (*this)[0];
        for (Index i = 1; i < size(); ++i) {
            if ((*this)[i] < min_val) min_val = (*this)[i];
        }
        return min_val;
    }
    
    Scalar maxCoeff() const {
        if (size() == 0) return 0;
        Scalar max_val = (*this)[0];
        for (Index i = 1; i < size(); ++i) {
            if ((*this)[i] > max_val) max_val = (*this)[i];
        }
        return max_val;
    }
    
    Matrix cwiseProduct(const Matrix& other) const {
        if (this->rows() != other.rows() || this->cols() != other.cols_) {
            return Matrix();
        }
        Matrix result(this->rows(), this->cols());
        for (Index i = 0; i < this->size(); ++i) {
            result.data()[i] = (*this)[i] * other.data()[i];
        }
        return result;
    }
    
    Scalar squaredNorm() const {
        Scalar result = 0;
        for (Index i = 0; i < size(); ++i) {
            result += (*this)[i] * (*this)[i];
        }
        return result;
    }
    
    // Static constructors for constant vectors
    static Vector Constant(Index size, Scalar value) {
        Vector result(size);
        for (Index i = 0; i < size; ++i) {
            result[i] = value;
        }
        return result;
    }
    
    // Cross product (for 3D vectors)
    Vector cross(const Vector& other) const {
        if (size() != 3 || other.size() != 3) {
            return Vector();
        }
        Vector result(3);
        result[0] = (*this)[1] * other[2] - (*this)[2] * other[1];
        result[1] = (*this)[2] * other[0] - (*this)[0] * other[2];
        result[2] = (*this)[0] * other[1] - (*this)[1] * other[0];
        return result;
    }
    
    // Static constructors
    static Vector Zero(Index size) {
        Vector result(size);
        result.setZero();
        return result;
    }
    
    static Vector Ones(Index size) {
        Vector result(size);
        result.setOnes();
        return result;
    }
    
    static Vector Random(Index size) {
        Vector result(size);
        result.setRandom();
        return result;
    }
};

// Type aliases
using MatrixXd = Matrix<double>;
using VectorXd = Vector<double>;
using MatrixXf = Matrix<float>;
using VectorXf = Vector<float>;

} // namespace Eigen 