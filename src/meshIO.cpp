#include "meshIO.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <time.h>

#define BUFFER_LENGTH 256

using namespace std;

/**
 * @brief Seperate string origin by given a set of patterns.
 * 
 * @param origin 
 * @return std::vector<std::string> 
 */
std::vector<std::string> seperate_string(std::string origin) {
    std::vector<std::string> result;
    stringstream ss(origin);
    while(ss >> origin) result.push_back(origin);
    return result;
}

// The eps file format is 
/* eps 0.005          
 * id 1 5 7 9 100
 * eps 0.008
 * id 56 78 79
 * ...
 */
int MESHIO::readEPS(std::string filename, int& cou, std::map<int, double>& mpd, std::map<int, vector<int>>& mpi) {
	std::ifstream eps_file;
	eps_file.open(filename);
	if(!eps_file.is_open()){
		std::cout << "No Such file. - " << filename << std::endl;
		return -1;
	}
	string line;
	string _;
	stringstream input;
    // int cou = 0;
    // std::map<int, double> mpd;
    // std::map<int, vector<int>> mpi;
	while(getline(eps_file, line)){
		input.clear();
		input.str(line);
		input >> _;
		if(_[0] == '#') continue;
		else if(_ == "eps" || _ == "EPS"){
            cou++;
			input >> mpd[cou];
		}
		else if(_ == "id" || _ == "ID"){
			int tmpId;
			while (input >> tmpId)
			{
				mpi[cou].push_back(tmpId);
			}
		}
		else{
			continue;
		}
	}
	return 0;
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
    char buffer[BUFFER_LENGTH];
    while(!vtk_file.eof()) {
        vtk_file.getline(buffer, BUFFER_LENGTH);
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
                vtk_file.getline(buffer, BUFFER_LENGTH);
                words = seperate_string(std::string(buffer));
                V.row(i) << stod(words[0]), stod(words[1]), stod(words[2]);
            }
        }
        if(line.find(vtk_type_str) != std::string::npos) {
            std::vector<std::string> words = seperate_string(line);
            nFacets = stoi(words[1]);
            T.resize(nFacets, stoi(words[2]) / nFacets - 1);
            for(int i = 0; i < nFacets; i++) {
                vtk_file.getline(buffer, BUFFER_LENGTH);
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
            vtk_file.getline(buffer, BUFFER_LENGTH);
            std::string data_type = seperate_string(std::string(buffer))[1];
            if(data_type != mark_pattern) 
                continue;
            M.resize(nFacets, 1);
			for (int i = 0; i < nFacets; i++)
				M(i, 0) = 0;
            vtk_file.getline(buffer, BUFFER_LENGTH);
            for(int i = 0; i < nFacets; i++) {
                vtk_file.getline(buffer, BUFFER_LENGTH);
				int surface_id=stoi(std::string(buffer));
				M.row(i) << surface_id;
					
            }
        }
    }
    vtk_file.close();
    return 1;
}

int MESHIO::readOBJ(std::string filename, Eigen::MatrixXd &V, Eigen::MatrixXi &T, Eigen::MatrixXi &M) {
    ifstream objFile;
    objFile.open(filename);
    if(!objFile.is_open()) {
        std::cout << "No such file. - " << filename << std::endl;
        return -1;
    }
    char buffer[BUFFER_LENGTH];
    vector<vector<double>> plist;
    vector<vector<int>> flist;
    vector<int> mlist;
    int curMark = 0;
    while(!objFile.eof()) {
        objFile.getline(buffer, BUFFER_LENGTH);
        if(buffer[0] == 'v') {
            vector<string> words = seperate_string(string(buffer));
            if(words.size() < 4) {
                continue;
            }
            vector<double> coord;
            for(int ii = 1; ii < 4; ii++) {
                int wordLen = 0;
                while(wordLen < words[ii].length()) {
                    if(words[ii][wordLen] == '/') {
                        break;
                    }
                }
                coord.push_back(stod(words[ii].substr(0, wordLen)));
            }
            plist.push_back(coord);
        }
        if(buffer[0] == 'f') {
            vector<string> words = seperate_string(string(buffer));
            if(words.size() < 4) {
                continue;
            }
            vector<int> facet;
            for(int ii = 1; ii < 4; ii++) {
                int wordLen = 0;
                while(wordLen < words[ii].length()) {
                    if(words[ii][wordLen] == '/') {
                        break;
                    }
                }
                facet.push_back(stoi(words[ii].substr(0, wordLen)));
            }
            flist.push_back(facet);
            mlist.push_back(curMark);
        }
        if(buffer[0] == 'g') {
            ++curMark;
        }
    }

    V.resize(plist.size(), 3);
    for(int i = 0; i < plist.size(); i++) {
        for(int j = 0; j < 3; j++) {
            V(i, j) = plist[i][j];
        }
    }
    T.resize(flist.size(), 3);
    for(int i = 0; i < flist.size(); i++) {
        for(int j = 0; j < 3; j++) {
            T(i, j) = flist[i][j];
        }
    }
    M.resize(mlist.size(), 1);
    for(int i = 0; i < mlist.size(); i++) {
        M(i, 0) = mlist[i];
    }

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
            f <<  T(i, j) << " ";
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

int MESHIO::writeEpsVTK(std::string filename, const Eigen::MatrixXd &V, const Eigen::MatrixXi &T, int& cou,  std::map<int, double> &mpd, std::map<int, vector<int>> &mpi, std::string mark_pattern) {
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
	f << "CELLS " << V.rows() << " " << V.rows() * 2 << std::endl;
	for(int i = 0; i < V.rows(); i++) {
		f << 1 <<  " " << i ;
		f << std::endl;
	}
	f << "CELL_TYPES " << V.rows() << std::endl;
	int cellType = 1;
	for(int i = 0; i < V.rows(); i++)
		f << cellType << std::endl;

	f << "CELL_DATA " << V.rows() << std::endl;
	f << "SCALARS local_epsilon double 1" << std::endl;
	f << "LOOKUP_TABLE default" << std::endl;

	int curLoc = 0;

    vector<double> vec;
    vec.resize(V.rows(), -1);
    for(int i = 1; i <= cou; i++)
    {
        for(int id : mpi[i]){
            vec[id] = mpd[i];
        }
    }

	for(int i = 0; i < V.rows(); i++) {
		f << vec[i] << endl;
        if(vec[i] != -1){
            std::cout << i << " " << vec[i] << '\n';
        }
	}
	f << std::endl;
	f.close();
	return 1;
}

int MESHIO::readMESH(std::string filename, Eigen::MatrixXd &V, Eigen::MatrixXi &T, Eigen::MatrixXi &M) {
    std::ifstream mesh_file;
    mesh_file.open(filename);
    if(!mesh_file.is_open()) {
        std::cout << "No such file. - " << filename << std::endl;
        return -1;
    }
    int dimension = 3;
    int nPoints;
    int nFacets;
    char buffer[BUFFER_LENGTH];
    while(!mesh_file.eof()) {
        mesh_file.getline(buffer, BUFFER_LENGTH);
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
                mesh_file.getline(buffer, BUFFER_LENGTH);
                line = (std::string)buffer;
            }
            words = seperate_string(line);
            nPoints = std::stoi(words[0]);
            std::cout << "Number of points : " << nPoints << std::endl;
            V.resize(nPoints, dimension);
            int i = 0;
            while(i < nPoints) {
                mesh_file.getline(buffer, BUFFER_LENGTH);
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
                mesh_file.getline(buffer, BUFFER_LENGTH);
                line = (std::string)buffer;
            }
            words = seperate_string(line);
            nFacets = stoi(words[0]);
            std::cout << "Number of facets : " << nFacets << std::endl;
            T.resize(nFacets, 3);
            M.resize(nFacets, 1);
            int i = 0;
            while(i < nFacets) {
                mesh_file.getline(buffer, BUFFER_LENGTH);
                words = seperate_string(std::string(buffer));
                if(words.size() < 4)
                {
                    std::cout << "Warning : The number of triangles element is not equal 4.\n";
                }
                for(int j = 0; j < 3; j++)
                    T(i, j) = std::stoi(words[j]) - 1;
                M(i, 0) = std::stoi(words[3]) - 1;
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
	plyfile.precision(std::numeric_limits<double>::digits10 + 1);
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

int MESHIO::writePLS(std::string filename, const Eigen::MatrixXd &V, const Eigen::MatrixXi &T, const Eigen::MatrixXi &M)
{
    if(T.cols() != 3)
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
        plsfile << i + 1 << " " << T(i, 0) + 1 << " " << T(i, 1) + 1 << " " << T(i, 2) + 1 << " " << M(i, 0) + 1 << std::endl;
        
    plsfile.close();
    std::cout << "Finish\n";
    return 1;
}

int MESHIO::readPLS(std::string filename, Eigen::MatrixXd &V, Eigen::MatrixXi &T,Eigen::MatrixXi &M) {
	std::ifstream plsfile;
	plsfile.open(filename);
	if (!plsfile.is_open()) {
		std::cout << "No such file. - " << filename << std::endl;
		return -1;
	}
	int nPoints = 0;
	int nFacets = 0;

	auto& pls_file = plsfile;
	std::string str;
	std::getline(pls_file, str);
	std::stringstream ss(str);
	ss >> nFacets >> nPoints;
	V.resize(nPoints,3);
	for (int i = 0; i < nPoints; i++) {
		int index;
		double point_coordinate[3];
		pls_file >> index >> point_coordinate[0] >> point_coordinate[1] >> point_coordinate[2];
		for(int k=0;k<3;k++)
		V(index-1,k)=point_coordinate[k];
	}
	M.resize(nFacets,1);
	T.resize(nFacets, 3);
	for (int i = 0; i < nFacets; i++) {
		int index;
		int tri[3];
		pls_file >> index >> tri[0] >> tri[1] >> tri[2] >> M(i,0);
		tri[0]--; tri[1]--; tri[2]--;
		for (int k = 0; k < 3; k++)
			T(i, k) = tri[k];
	}

	return 1;
}

int MESHIO::writeFacet(std::string filename, const Eigen::MatrixXd & V, const Eigen::MatrixXi & T, const Eigen::MatrixXi &M)
{
	std::cout << "Writing mesh to - " << filename << std::endl;
	std::ofstream facetfile;
	facetfile.open(filename);
	facetfile.precision(std::numeric_limits<double>::digits10 + 1);
	facetfile << "FACET FILE V3.0  exported from Meshconverter http://10.12.220.71/tools/meshconverter " << std::endl;
	facetfile << 1 << std::endl;
	facetfile << "Grid" << std::endl;
	facetfile << "0, 0.00 0.00 0.00 0.00" << endl;
	facetfile << V.rows() << endl;
	for (int i = 0; i < V.rows(); i++)
		facetfile << V(i, 0) << " " << V(i, 1) << " " << V(i, 2) << std::endl;
	facetfile << 1 << endl;
	facetfile << "Triangles" << endl;
	facetfile << T.rows() << " 3" << endl;
	
	for (int i = 0; i < T.rows(); i++) {
		facetfile << " " << T(i, 0) + 1 << " " << T(i, 1) + 1 << " " << T(i, 2) + 1 << " 0 ";
		if (M.rows() >= T.rows())
			facetfile << M(i, 0);
		else
			facetfile << 0;
		facetfile<< " " << i + 1 << std::endl;
	}
	return 0;
}

int MESHIO::writeOBJ(string filename, const Eigen::MatrixXd& V, const Eigen::MatrixXi& F, const Eigen::MatrixXi &M) {
    // Facet group
    bool doGroup = (M.rows() == F.rows());
    vector<vector<int>> flist;
    for(int i = 0; i < F.rows(); i++) {
        vector<int> facet;
        facet.push_back(doGroup ? M(i, 0) : 0);
        for(int j = 0; j < F.cols(); j++) {
            facet.push_back(F(i, j));
        }
        flist.push_back(facet);
    }
    if(doGroup) {
        sort(flist.begin(), flist.end(), [](vector<int> A, vector<int> B){ return A[0] < B[0]; });
    }

	cout << "Writing mesh to - " << filename << endl;
	ofstream objFile;
	objFile.open(filename);
	objFile.precision(std::numeric_limits<double>::digits10 + 1);

    // Get current time.
    std::string export_time;
    char stime[BUFFER_LENGTH] = {0};
    time_t now_time;
    time(&now_time);
    strftime(stime,sizeof(stime),"%H:%M:%S",localtime(&now_time));
    export_time = stime + '\0';

    // Header 
    objFile << "# TIGER Mesh converter. (c) 2021." << endl;
    objFile << "# Created File: " << export_time << endl;
    objFile << "# " << endl;
    objFile << "# object default" << endl;
    objFile << "# " << endl;
    objFile << endl;

    // Write points
    for(int i = 0; i < V.rows(); i++) {
        objFile << "v";
        for(int j = 0; j < V.cols(); j++) {
            objFile << " " << V(i, j);
        }
        objFile << endl;
    }
    objFile << "# " << V.rows() << " vertices" << endl << endl;

    // Write facets with groups
    int curGroup = INT_MIN;
    for(int i = 0; i < flist.size(); i++) {
        if(flist[i][0] != curGroup) {
            curGroup = flist[i][0];
            objFile << "g " << curGroup << endl;
        }
        objFile << "f";
        for(int j = 1; j < flist[i].size(); j++) {
            objFile << " " << flist[i][j] + 1;
        }
        objFile << endl;
    }

    // Write facets 
    objFile << "# " << F.rows() << " faces" << endl << endl;

    objFile.close();

    return 1;
}

/**
 * the start point is (start_x, start_y, start_z), the orient is (end_x, end_y, end_z), angle is PI * angle. angle \in (0, 2).
 * @param rotateVec is the param of rotate.
 * @param V
 * @param T
 * @return 1/0
 */
bool MESHIO::rotatePoint(vector<double> rotateVec, Eigen::MatrixXd &V, Eigen::MatrixXi &T)
{
	if(rotateVec.size() == 0) return 0;

	double start_x = 0.0, start_y = 0.0, start_z = 0.0;
	double end_x = 0.0, end_y = 0.0, end_z = 0.0;
	double angle = 0.0;
	if(rotateVec.size() > 0){
		if(rotateVec.size() == 4) {
			end_x = rotateVec[0];
			end_y = rotateVec[1];
			end_z = rotateVec[2];
			angle = rotateVec[3];
		}
		else if(rotateVec.size() == 7){
			start_x = rotateVec[0]; start_y = rotateVec[1]; start_z = rotateVec[2];
			end_x = rotateVec[3]; end_y = rotateVec[4]; end_z = rotateVec[5];
			angle = rotateVec[6];
		}else{
			std::cout << "The format is Error.Format is (start_x, start_y, start_z, end_x, end_y, end_z, angle) or (end_x, end_y, end_z, angle). angle value scale is (0, 2).";
			return -1;
		}
	}

	if(rotateVec.size() == 7) {
		std::cout << "Rotating\n";
		for (int i = 0; i < V.rows(); i++) {
			V(i, 0) -= start_x;
			V(i, 1) -= start_y;
			V(i, 2) -= start_z;
		}
	}
	if(rotateVec.size() > 0) {
		Eigen::AngleAxisd rotationVector(M_PI * angle, Eigen::Vector3d(end_x, end_y, end_z));
		Eigen::Matrix3d rotationMatrix = Eigen::Matrix3d::Identity();
		rotationMatrix = rotationVector.toRotationMatrix();
		Eigen::MatrixXd tmp = rotationMatrix * V.transpose();
		V = tmp.transpose();
	}
	if(rotateVec.size() == 7){
		for(int i = 0; i < V.rows(); i++){
			V(i, 0) += start_x;
			V(i, 1) += start_y;
			V(i, 2) += start_z;
		}
		std::cout << "Rotated\n";
	}
	return 1;
}

/**
 * Add bounding box.
 * @param boxVec
 * @param V
 * @param T
 * @return
 */
bool MESHIO::addBox(vector<double> boxVec, Eigen::MatrixXd &V, Eigen::MatrixXi &T, Eigen::MatrixXi &M)
{
	if(boxVec.size() == 0) return 0;

	if(boxVec.size() != 3) {
		std::cout << "the number of boxVec's param is not equal to 3." << std::endl;
		return 0;
	}


	std::cout << "Boxing\n";

	double len = boxVec[0], width = boxVec[1], hight = boxVec[2];

	vector<double> tmpx, tmpy, tmpz;

	for(int i = 0; i < V.rows(); i++)
	{
		tmpx.push_back(V(i, 0));
		tmpy.push_back(V(i, 1));
		tmpz.push_back(V(i, 2));
	}

	std::sort(tmpx.begin(), tmpx.end());
	std::sort(tmpy.begin(), tmpy.end());
	std::sort(tmpz.begin(), tmpz.end());

	int tmpid = V.rows() / 2;

	double midx = tmpx[tmpid];
	double midy = tmpy[tmpid];
	double midz = tmpz[tmpid];

	std::cout << midx << " " << midy << " " << midz << '\n';
	std::cout << tmpx[tmpx.size() - 1] - tmpx[0] << " " << tmpy[tmpy.size() - 1] - tmpy[0] << " " << \
	tmpz[tmpz.size() - 1] - tmpz[0] << '\n';

	tmpx.clear(); tmpy.clear(); tmpz.clear();


	Eigen::MatrixXd tmpV;
	tmpV.resize(V.rows() + 8, V.cols());
	for(int i = 0; i < V.rows(); i++)
		for(int j = 0; j < V.cols(); j++)
			tmpV(i, j) = V(i, j);

	Eigen::MatrixXi tmpT;
	tmpT.resize(T.rows() + 12, T.cols());
	for(int i = 0; i < T.rows(); i++)
		for(int j = 0; j < T.cols(); j++)
			tmpT(i, j) = T(i, j);

	Eigen::MatrixXi tmpM;
	tmpM.resize(M.rows() + 12, M.cols());
	for(int i = 0; i < tmpM.cols(); i++)
		tmpM(i, 0) = M(i, 0);


	int V_index = V.rows();
	for(double k = hight / 2; k > -hight ; k -= hight) // up and down
	{
		for(double j = width / 2; j > -width; j -= width) // left and right
		{
			for(double i = len / 2; i > -len; i -= len) // forward and back
			{
				tmpV(V_index, 0) = midx + i;
				tmpV(V_index, 1) = midy + j;
				tmpV(V_index, 2) = midz + k;
				V_index ++;
			}
		}
	}


	/***************************
	 *    4 _________2
	 *     /|        /|
	 *   3/_|______1/ |
	 *    | |8_____|__|6
	 *    | /      | /
	 *  7 |/_______|/5
	 ****************************/

	V_index = V.rows() - 1;
	vector<int> boxTri = {
			1, 4, 2, // up
			1, 3, 4, // up
			5, 6, 8, // down
			5, 8, 7, // down
			5, 3, 1, // front
			5, 7, 3, // front
			6, 2, 4, // behind
			6, 4, 8, // behind
			7, 8, 4, // left
			7, 4, 3, // left
			5, 1, 2, // right
			5, 2, 6  // right
	};

	for(int i = T.rows(), j = 0; i < T.rows() + 12; i++, j += 3){
		tmpT(i, 0) = V_index + boxTri[j + 0];
		tmpT(i, 1) = V_index + boxTri[j + 1];
		tmpT(i, 2) = V_index + boxTri[j + 2];
		tmpM(i, 0) = M(T.rows() - 1, 0) + 1;
	}


	V.resize(tmpV.rows(), tmpV.cols());
	T.resize(tmpT.rows(), tmpT.cols());
	V = tmpV;
	T = tmpT;

	std::cout << "Boxed\n";

	return 1;
}

bool MESHIO::reverseOrient(Eigen::MatrixXi &T) {
	std::cout << "Reversing\n";
	for(int i = 0; i < T.cols(); i++)
	{
		int t = T(i, 0);
		T(i, 0) = T(i, 2);
		T(i, 2) = t;
	}
	std::cout << "reversed\n";
	return 1;
}

bool MESHIO::repair( Eigen::MatrixXd &V,  Eigen::MatrixXi &T, Eigen::MatrixXi M)
{

    std::cout << "Vertex number is  " << V.rows() << " X " << V.cols() << "  before clean. \n";
    std::cout << "Cell number is  " << T.rows() << " X " << T.cols() << "  before clean. \n";
    std::cout << "Attribute number is  " << M.rows() << " X " << M.cols() << "  before clean. \n";
    // 检测面积为0的单元个数

    Eigen::MatrixXd Tri;
    Tri.resize(3, 3);
    vector<int> emptyTri;
    for(int i = 0; i < T.rows(); i++)
    {
        for(int j = 0; j < 3; j++)
        {
            Tri(j, 0) = V(T(i, j), 0);
            Tri(j, 1) = V(T(i, j), 1);
            Tri(j, 2) = V(T(i, j), 2);
        }
        Eigen::Vector3d lin1(Tri(1, 0) - Tri(0, 0), Tri(1, 1) - Tri(0, 1), Tri(1, 2) - Tri(0, 2));
        Eigen::Vector3d lin2(Tri(2, 0) - Tri(0, 0), Tri(2, 1) - Tri(0, 1), Tri(2, 2) - Tri(0, 2));

        Eigen::Vector3d crossResult = lin1.cross(lin2);
        double area = crossResult.norm();
        if(area < 1e-8)
        {
            emptyTri.push_back(i);
        }
    }

    std::cout << "There are " << emptyTri.size() << " cells whose area is equal to zero.\n";

    // 去除面积为0的单元

    struct node
    {
        int oldid;
        Eigen::Vector3d point;
    };
    vector<node> vec(V.rows(), node());
    map<int, int> mpid;

    for(int i = 0; i < V.rows(); i++)
    {
        vec[i].oldid = i;
        vec[i].point = V.row(i);
    }

    std::sort(vec.begin(), vec.end(), [](const node& a, const node& b)
    {
        if( a.point.x() != b.point.x() ) return a.point.x() < b.point.x();
        if( a.point.y() != b.point.y() ) return a.point.y() < b.point.y();
        if( a.point.z() != b.point.z() ) return a.point.z() < b.point.z();
        return a.oldid < b.oldid;
    });

    int curid = 0;
    for(int i = 0; i < vec.size(); i++)
    {
        while( i + 1 < vec.size() && (vec[i].point - vec[i + 1].point).norm() < 1e-8 )
        {
            mpid[vec[i].oldid] = curid;
            i++;
        }
        mpid[vec[i].oldid] = curid;
        curid++;
    }

    V.resize(curid, V.cols());

    for(int i = 0; i < vec.size(); i++)
    {
        V.row(mpid[ vec[i].oldid ]) = vec[i].point;
        while( i + 1 < vec.size() && (vec[i].point - vec[i + 1].point).norm() < 1e-8 ) i++;
    }

    Eigen::MatrixXi T2;
    Eigen::MatrixXi M2;
    T2.resize(T.rows() - emptyTri.size(), T.cols());
    M2.resize(T.rows() - emptyTri.size(), M.cols());

    int locT2 = 0, locEmptyTri = 0;
    for(int i = 0; i < T.rows(); i++)
    {
        if(emptyTri[locEmptyTri] == i)
        {
            locEmptyTri++;
            continue;
        }
        for(int j = 0; j < T.cols(); j++)
        {
            T2(locT2, j) = mpid[T(i, j)];
        }
        locT2++;
    }

    T = T2;
    // M = M2;

    std::cout << "Vertex number is  " << V.rows() << " X " << V.cols() << "  after clean. \n";
    std::cout << "Cell number is  " << T.rows() << " X " << T.cols() << "  after clean. \n";
    std::cout << "Attribute number is  " << M.rows() << " X " << M.cols() << "  after clean. \n";
    std::cout << "Clean all cell whose area is equal to zero. " << '\n';
	return 0;
}
