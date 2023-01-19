#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>


#include "TypeDefs.h"

#include "third-party/mm_io.h"

#if PPP_USE_EIGEN
#include <Eigen/Sparse>
#endif

namespace PPP
{

    /// \ingroup PPP
    /** lock-free parallel sparse matrix data structure
     * consider: VectorType<std::shared_ptr<std::unordered_map<ItemIndexType, T>>>
     * */
    template <typename T> class SparseMatrix
    {
    public:
        typedef std::vector<std::pair<ItemIndexType, T>> RowType;
        typedef std::pair<ItemIndexType, T> ItemType;

    private:
        VectorType<std::shared_ptr<std::vector<std::pair<ItemIndexType, T>>>> mat;
        // size_t myRowCount;
        std::string myMatrixShape;

    public:
        SparseMatrix() = default;
        explicit SparseMatrix(const ItemIndexType rowCount)
        {
            resize(rowCount);
        }

        void resize(const ItemIndexType rowCount)
        {
            mat.reserve(rowCount); // still need init each index
            for (std::size_t j = 0; j < rowCount; j++)
            {
                mat.push_back(std::make_shared<std::vector<std::pair<ItemIndexType, T>>>());
            }
        }

        inline size_t rowCount() const
        {
            return mat.size();
        }

        /// not thread-safe, other thread may insert item
        size_t elementSize() const
        {
            size_t n = 0;
            for (std::size_t j = 0; j < mat.size(); j++)
            {
                n += mat[j]->size();
            }
            return n;
        }

        /// return the reference of the rowIndex
        RowType& operator[](const size_t rowIndex)
        {
            return (*mat[rowIndex]);
        }

        /// return the const reference of the rowIndex
        const RowType& operator[](const size_t rowIndex) const
        {
            return (*mat[rowIndex]);
        }

        bool hasElement(const size_t row, const size_t col) const
        {
            if (mat.size() >= row)
            {
                for (const auto& it : (*mat[row]))
                {
                    if (col == it.first)
                        return true;
                }
            }
            return false;
        }

        T getElement(const size_t row, const size_t col) const
        {
            if (mat.size() >= row)
            {
                for (const auto& it : (*mat[row]))
                {
                    if (col == it.first)
                        return it.second;
                }
            }
            throw std::out_of_range("item not exists or index out of range");
        }

        void insertAt(const size_t row, const size_t col, const T val)
        {
            if (mat.size() < row)
            {
                throw std::runtime_error("row index out of range, have you resize the matrix before indexing?");
            }
            (*mat[row]).push_back(std::make_pair(col, val));
        }


        void removeAt(const size_t row, const size_t col)
        {
            if (mat.size() > row)
            {
                RowType& r = (*mat[row]);
                for (auto it = r.begin(); it < r.end(); it++)
                {
                    if (it->first == col)
                    {
                        r.erase(it);
                        break;
                    }
                }
            }
        }


        ///  no repeating index pair, so the actually elementSize can be different
        static SparseMatrix random(const size_t Nrows, const size_t elementSize)
        {
            std::random_device rd;  // Will be used to obtain a seed for the random number engine
            std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
            std::uniform_int_distribution<> dis(0, Nrows - 1); // can generate the upper boundary value

            SparseMatrix A;
            A.resize(Nrows);
            size_t perRow = elementSize / Nrows;

            for (std::size_t i = 0; i < Nrows; i++)
            {
                std::set<size_t> s;
                for (auto ii = 0; ii < perRow; ii++)
                    s.insert(dis(gen));
                for (auto& j : s)
                    A[i].push_back(std::make_pair(j, T())); // T(), default value, false for bool
            }
            return A;
        }


        /// write sparse matrix into a json format of list of dict
        /// [{"colIndex": value, "colIndex": value,}, ... ]
        void toJson(const std::string file_name) const
        {
            /*  tested but only for json textual format */
            std::ofstream os(file_name);
            os << "[\n";
            os << std::setw(4);
            for (size_t i = 0; i < mat.size(); i++) // for each row
            {
                json jrow = json();
                os << std::setw(4);
                const auto& row = *mat[i]; // null will be saved for empty row
                for (const auto& it : row)
                    jrow[std::to_string(it.first)] = it.second;
                os << jrow;
                if (i != mat.size() - 1)
                    os << ",\n";
            }
            os << "\n]";
        }


        static SparseMatrix readMatrixMarketFile(const std::string& file_name)
        {
            using namespace std;
            FILE* f = fopen(file_name.c_str(), "r");
            MM_typecode matcode;
            int ret_code = 0;
            int M, N, nz;
            int I, J;

            if (mm_read_banner(f, &matcode) != 0)
            {
                cout << "Error: can not read Matrix Market banner for: " << file_name << endl;
            }
            if (mm_is_complex(matcode) && mm_is_matrix(matcode) && mm_is_sparse(matcode))
            {
                cout << "Market Market type: " << mm_typecode_to_str(matcode) << ", is not supported\n";
            }

            /* find out size of sparse matrix .... */
            if ((ret_code = mm_read_mtx_crd_size(f, &M, &N, &nz)) != 0)
                cout << "Error in reading Market Market size" << endl;

            SparseMatrix Mat(M);

            double _val = 0.0;
            for (int i = 0; i < nz; i++)
            {
                fscanf(f, "%d %d %lg\n", &I, &J, &_val); // assume value is float point type
                Mat.insertAt(I - 1, J - 1, T(_val));     /* adjust from 1-based to 0-based */
            }

            if (f != stdin)
                fclose(f);

            return Mat;
        }

        void writeMatrixMarketFile(const std::string& file_name) const
        {
            using namespace std;
            FILE* f = fopen(file_name.c_str(), "w");
            MM_typecode matcode;
            mm_initialize_typecode(&matcode);
            mm_set_matrix(&matcode);
            mm_set_coordinate(&matcode); // sparse pattern, i,j: value
            mm_set_symmetric(&matcode);  // matrix type
            mm_set_integer(&matcode);    // value type

            mm_write_banner(f, matcode);
            auto Nrows = int(this->rowCount());
            mm_write_mtx_crd_size(f, Nrows, Nrows, int(elementSize()));

            /* NOTE: matrix market files use 1-based int type indexing, i.e. first element
            of a vector has index 1, not 0.  */
            for (int i = 0; i < Nrows; i++)
            {
                for (const auto& it : (*mat[i])) // can save lots of basic numerical types, even std::string
                    fprintf(f, "%d %d %s\n", int(i + 1), int(it.first + 1), std::to_string(it.second).c_str());
            }
            fclose(f);
        }
    };


#if PPP_USE_EIGEN
    typedef Eigen::SparseMatrix<bool> AdjacencyMatrixType;
#else
/// extra export instruction for template instantiation for MSVC
#if defined(_MSC_VER)
    template class __declspec(dllexport) SparseMatrix<bool>;
#endif
    typedef SparseMatrix<bool> AdjacencyMatrixType;
#endif


} // namespace PPP