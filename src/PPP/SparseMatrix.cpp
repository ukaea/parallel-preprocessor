/// this cpp file is needed to hide MatrixMarket file IO impl and to avoid symbol exporting
/// https://www.codeproject.com/Articles/48575/How-to-Define-a-Template-Class-in-a-h-File-and-Imp

#include "SparseMatrix.h"
#include "third-party/mm_io.h"


namespace PPP
{

    template <typename T> SparseMatrix<T> SparseMatrix<T>::readMatrixMarketFile(const std::string& file_name)
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
            cout << "Market Market type: " << mm_typecode_to_str(matcode) << ", is not supproted\n";
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

    template <typename T> void SparseMatrix<T>::writeMatrixMarketFile(const std::string& file_name) const
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

} // namespace PPP