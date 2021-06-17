#include "meshIO.h"
#include <iostream>
#include <fstream>

/**
 * @brief Seperate string origin by given a set of patterns.
 * 
 * @param origin 
 * @param patterns If it meets one of the patterns, delete the charactor and split it from this index.
 * @return std::vector<std::string> 
 */
std::vector<std::string> seperate_string(std::string origin, std::vector<std::string> patterns = {" ", "\t"}) {
    std::vector<std::string> result;
    if (origin.length() == 0) {
        return result;
    }
    origin += patterns[0];
    size_t startPos = origin.npos;
    for (auto pt = patterns.begin(); pt != patterns.end(); pt++) {
        startPos = (origin.find(*pt) < startPos) ? origin.find(*pt) : startPos;
    }
    size_t pos = startPos;
    while (pos != origin.npos) {
        std::string temp = origin.substr(0, pos);
        if (temp.length())
            result.push_back(temp);
        origin = origin.substr(pos + 1, origin.size());
        pos = origin.npos;
        for (auto pt = patterns.begin(); pt != patterns.end(); pt++) {
            pos = (origin.find(*pt) < pos) ? origin.find(*pt) : pos;
        }
    }
    return result;
}

int MESHIO::readVTK(std::string filename, Eigen::MatrixXd &V, Eigen::MatrixXi &T, Eigen::MatrixXi &M, std::string mark_pattern) {
    M.resize(1, 1);
    int nPoints = 0;
    int nFacets = 0;
    std::ifstream vtk_file;
    vtk_file.open(filename);
    if(!vtk_file.is_open()) {
        std::cout << "No such file. - " << filename << std::endl;
        return -1;
    }
    std::string vtk_type_str = "POLYDATA ";
    char buffer[256];
    while(!vtk_file.eof()) {
        vtk_file.getline(buffer, 256);
        std::string line = (std::string)buffer;
        if(line.length() < 2 || buffer[0] == '#')
            continue;
        if(line.find("DATASET") != std::string::npos) {
            std::vector<std::string> words = seperate_string(line);
            if(words[1] == "POLYDATA") 
                vtk_type_str = "POLYGONS ";
            else if(words[1] == "UNSTRUCTURED_GRID")
                vtk_type_str = "CELLS ";
            else {
                std::cout << "The format of VTK file is illegal, No clear DATASET name. - " << filename << std::endl;
            }
        }
        if(line.find("POINTS ") != std::string::npos) {
            std::vector<std::string> words = seperate_string(line);
            nPoints = stoi(words[1]);
            V.resize(nPoints, 3);
            for(int i = 0; i < nPoints; i++) {
                vtk_file.getline(buffer, 256);
                words = seperate_string(std::string(buffer));
                V.row(i) << stod(words[0]), stod(words[1]), stod(words[2]);
            }
        }
        if(line.find(vtk_type_str) != std::string::npos) {
            std::vector<std::string> words = seperate_string(line);
            nFacets = stoi(words[1]);
            T.resize(nFacets, stoi(words[2]) / nFacets - 1);
            for(int i = 0; i < nFacets; i++) {
                vtk_file.getline(buffer, 256);
                words = seperate_string(std::string(buffer));
                for(int j = 0; j < stoi(words[0]); j++) 
                    T(i, j) = stoi(words[j + 1]);
            }
        }
        if(line.find("CELL_DATA ") != std::string::npos) {
            std::vector<std::string> words = seperate_string(line);
            if(stoi(words[1]) != nFacets) {
                std::cout << "The number of CELL_DATA is not equal to number of cells. -" << filename;
                std::cout << "Ignore CELL_DATA" << std::endl;
                return 0;
            }
            vtk_file.getline(buffer, 256);
            std::string data_type = seperate_string(std::string(buffer))[1];
            if(data_type != mark_pattern) 
                continue;
            M.resize(nFacets, 1);
            vtk_file.getline(buffer, 256);
            for(int i = 0; i < nFacets; i++) {
                vtk_file.getline(buffer, 256);
                M.row(i) << stoi(std::string(buffer));
            }
        }
    }
    vtk_file.close();
    return 1;
}

int MESHIO::writeVTK(std::string filename, const Eigen::MatrixXd &V, const Eigen::MatrixXi &T, const Eigen::MatrixXi M, std::string mark_pattern) {
    std::ofstream f(filename);
    if(!f.is_open()) {
        std::cout << "Write VTK file failed. - " << filename << std::endl;
        return -1;
    }
    std::cout << "Writing mesh to - " << filename << std::endl;
    f.precision(std::numeric_limits<double>::digits10 + 1);
    f << "# vtk DataFile Version 2.0" << std::endl;
    f << "TetWild Mesh" << std::endl;
    f << "ASCII" << std::endl;
    f << "DATASET UNSTRUCTURED_GRID" << std::endl;
    f << "POINTS " << V.rows() << " double" << std::endl;
    for(int i = 0; i < V.rows(); i++)
        f << V(i, 0) << " " << V(i, 1) << " " << V(i, 2) << std::endl;
    f << "CELLS " << T.rows() << " " << T.rows() * (T.cols() + 1) << std::endl;
    for(int i = 0; i < T.rows(); i++) {
        f << T.cols() << " ";
        for(int j = 0; j < T.cols(); j++)
            f << T(i, j) << " ";
        f << std::endl;
    }
    f << "CELL_TYPES " << T.rows() << std::endl;
    int cellType = 0;
    if(T.cols() == 2)
        cellType = 3;
    else if(T.cols() == 3)
        cellType = 5;
    else if(T.cols() == 4)
        cellType = 10;
    for(int i = 0; i < T.rows(); i++)
        f << cellType << std::endl;
    if(M.rows() != T.rows()) {
        f.close();
        return 1;
    }
    f << "CELL_DATA " << M.rows() << std::endl;
    f << "SCALARS " << mark_pattern << " int " << M.cols() << std::endl;
    f << "LOOKUP_TABLE default" << std::endl;
    for(int i = 0; i < M.rows(); i++) {
        for(int j = 0; j < M.cols(); j++)
            f << M(i, j);
        f << std::endl;
    }
    f << std::endl;
    f.close();
    return 1;
}

int MESHIO::readMESH(std::string filename, Eigen::MatrixXd &V, Eigen::MatrixXi &T) {
    std::ifstream mesh_file;
    mesh_file.open(filename);
    if(!mesh_file.is_open()) {
        std::cout << "No such file. - " << filename << std::endl;
        return -1;
    }
    int dimension = 3;
    int nPoints;
    int nFacets;
    char buffer[256];
    while(!mesh_file.eof()) {
        mesh_file.getline(buffer, 256);
        std::string line = (std::string)buffer;
        if(line.length() < 2)
            continue;
        std::vector<std::string> words = seperate_string(line);
        if(words.empty())
            continue;
        if(line.find("Dimension") != std::string::npos) {
            dimension = stoi(words[1]);
            std::cout << "Reading mesh dimension - " << dimension << std::endl;
        }
        if(line == "Vertices") {
            words = seperate_string(line);
            if(words.size() > 1 || words[0].length() > 8)
                continue;
            line.clear();
            while(line.empty()) {
                mesh_file.getline(buffer, 256);
                line = (std::string)buffer;
            }
            words = seperate_string(line);
            nPoints = std::stoi(words[0]);
            std::cout << "Number of points : " << nPoints << std::endl;
            V.resize(nPoints, dimension);
            int i = 0;
            while(i < nPoints) {
                mesh_file.getline(buffer, 256);
                words = seperate_string(std::string(buffer));
                if(words.size() != (dimension + 1)) continue;
                for(int j = 0; j < dimension; j++)
                    V(i, j) = std::stod(words[j]);
                i++;
            }
        }
        if(line.find("Triangles") != std::string::npos) {
            line.clear();
            while(line.empty()) {
                mesh_file.getline(buffer, 256);
                line = (std::string)buffer;
            }
            words = seperate_string(line);
            nFacets = stoi(words[0]);
            std::cout << "Number of facets : " << nFacets << std::endl;
            T.resize(nFacets, 4);
            int i = 0;
            while(i < nFacets) {
                mesh_file.getline(buffer, 256);
                words = seperate_string(std::string(buffer));
                if(words.size() < 4)
                {
                    std::cout << "Warning : The number of triangles element is not equal 4.\n";
                }
                for(int j = 0; j < 4; j++)
                    T(i, j) = std::stoi(words[j]) - 1;
                i++;
            }
        }
    }
    mesh_file.close();
    return 1;
}

int MESHIO::writeMESH(std::string filename, const Eigen::MatrixXd &V, const Eigen::MatrixXi &T) {
    std::ofstream f(filename);
    if(!f.is_open()) {
        std::cout << "Write MESH file failed. - " << filename << std::endl;
        return -1;
    }
    std::cout << "Writing mesh to - " << filename << std::endl;
    f.precision(std::numeric_limits<double>::digits10 + 1);
    f << "MeshVersionFormatted 1" << std::endl;
    f << "Dimension " << V.cols() << std::endl;
    f << "Vertices" << std::endl;
    f << V.rows() << std::endl;
    for(int i = 0; i < V.rows(); i++) {
        for(int j = 0; j < V.cols(); j++)
            f << V(i, j) << " ";
        f << i + 1 << std::endl;
    }
    if(T.cols() == 3)
        f << "Triangles" << std::endl;
    else if(T.cols() == 4)
        f << "Tetrahedra" << std::endl;
    else {
        std::cout << "Unsupported format for .mesh file." << std::endl;
        return -1;
    }
    f << T.rows() << std::endl;
    for(int i = 0; i < T.rows(); i++) {
        for(int j = 0; j < T.cols(); j++)
            f << T(i, j) + 1 << " ";
        f << i + 1 << std::endl;
    }
    f.close();
    return 1;
}
int MESHIO::writePLY(std::string filename, const Eigen::MatrixXd &V, const Eigen::MatrixXi &T) {
    if(T.cols() != 3) {
        std::cout << "Unsupported format for .ply file." << std::endl;
        return -1;
    }
    std::cout << "Writing mesh to - " << filename << std::endl;
    std::ofstream plyfile;
    plyfile.open(filename);
    plyfile << "ply" << std::endl;
    plyfile << "format ascii 1.0" << std::endl;
    plyfile << "comment VTK generated PLY File" << std::endl;
    plyfile << "obj_info vtkPolyData points and polygons: vtk4.0" << std::endl;
    plyfile << "element vertex " << V.rows() << std::endl;
    plyfile << "property float x" << std::endl;
    plyfile << "property float y" << std::endl;
    plyfile << "property float z" << std::endl;
    plyfile << "element face " << T.rows() << std::endl;
    plyfile << "property list uchar int vertex_indices" << std::endl;
    plyfile << "end_header" << std::endl;
    for(int i = 0; i < V.rows(); i++)
        plyfile << V(i, 0) << " " << V(i, 1) << " " << V(i, 2) << std::endl;
    for(int i = 0; i < T.rows(); i++)
        plyfile << T.cols() << " " << T(i, 0) << " " << T(i, 1) << " " << T(i, 2) << std::endl;
    plyfile.close();
    return 1;
}


int MESHIO::writePLS(std::string filename, const Eigen::MatrixXd &V, const Eigen::MatrixXi &T)
{
    if(T.cols() != 4)
    {
        std::cout << "Unsupported format for .pls file." << std::endl;
        return -1;
    }
    std::cout << "Writing mesh to - " << filename << std::endl;
    std::ofstream plsfile;
    plsfile.open(filename);
    plsfile << T.rows() << " " << V.rows() << " " << "0 0 0 0\n";
    for(int i = 0; i < V.rows(); i++)
        plsfile << i + 1 << " " << V(i, 0) << " " << V(i, 1) << " " << V(i, 2) << std::endl;
    for(int i = 0; i < T.rows(); i++)
        plsfile << i + 1 << " " << T(i, 0) << " " << T(i, 1) << " " << T(i, 2) << " " << T(i, 3) << std::endl; 
        
    plsfile.close();
    std::cout << "Finish\n";
    return 1;
}