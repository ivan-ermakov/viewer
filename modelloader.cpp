#include "model.h"
#include "modelloader.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <locale>

#include <QMutexLocker>
#include <QProgressDialog>

struct Ctype: std::ctype<char>
{
    Ctype(): std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table()
    {
        typedef std::ctype<char> cctype;
        static const cctype::mask *const_rc= cctype::classic_table();

        static cctype::mask rc[cctype::table_size];
        std::memcpy(rc, const_rc, cctype::table_size * sizeof(cctype::mask));

        rc['/'] = std::ctype_base::space;
        rc[' '] = std::ctype_base::space;
        return &rc[0];
    }
};



ModelLoader::ModelLoader(Model* mdl_ = 0) :
    QThread(mdl_)
{
    mdl = mdl_;
    cancelled = false;
}

void ModelLoader::cancel()
{
    cancelled = true;
}

void ModelLoader::run()
{
    QMutexLocker lck(&(mdl->mutex));
    std::vector<QVector3D> faceNormals;
    std::vector<Vertex> vdata;
    std::vector<GLuint> indices;
    mdl->pivot = QVector3D(0, 0, 0);
    mdl->modelFile;

    std::ifstream in(mdl->modelFile.toStdString(), std::ios::in);
    if (!in)
    {
        std::cerr << "Cannot open " << mdl->modelFile.toStdString() << std::endl;
        return;
    }

    int last_progress = 0;
    std::size_t size;
    in.seekg(0, std::ios::end);
    size = in.tellg();
    in.seekg(0, std::ios::beg);

    setMaxProgress(size);
    //mdl->progress->setMaximum(size);

    bool precomputedNormals = false;
    bool texturePresent = false;
    int perFace = 3;
    float x, y, z;
    GLuint a, b, c, d, n, n2, n3, n4;
    Vertex vtx;
    QVector3D v;
    std::string line;
    std::string s;
    std::stringstream ss;

    ss.imbue(std::locale(std::locale(), new Ctype()));

    while (std::getline(in, s))
    {
        ss.clear();
        ss.str(s);
        ss >> line;

        if (line == "v")
        {
            ss >> x >> y >> z;

            vtx.pos.setX(x); vtx.pos.setY(y); vtx.pos.setZ(z);
            vdata.push_back(vtx);

            mdl->pivot += vtx.pos;
        }
        else if (line == "f")
        {
             ss >> a;
             if (texturePresent) ss >> n;
             if (precomputedNormals) ss >> n;

             if (ss.fail())
             {
                 std::cerr << "ERROR: invalid model file.\n";
                 return;
             }

             //std::cout << s << "\t" << a << "/" << n << "\t";

             ss >> b;
             if (texturePresent) ss >> n2;
             if (precomputedNormals) ss >> n2;

             if (ss.fail())
             {
                 std::cerr << "ERROR: invalid model file.\n";
                 return;
             }
             //std::cout << b << "/" << n2 << "\t";

             ss >> c;
             if (texturePresent) ss >> n3;
             if (precomputedNormals) ss >> n3;
             //std::cout << c << "/" << n3 << "\n";

             if (ss.fail())
             {
                 std::cerr << "ERROR: invalid model file.\n";
                 return;
             }

             ss >> d;

             if (!ss.fail())
             {
                 perFace = 4;

                 if (texturePresent) ss >> n4;
                 if (precomputedNormals) ss >> n4;
             }

             indices.push_back(--a);
             indices.push_back(--b);
             indices.push_back(--c);

             vdata[a].norm += faceNormals[--n];
             vdata[b].norm += faceNormals[--n2];
             vdata[c].norm += faceNormals[--n3];

            if (perFace == 4)
            {
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(--d);

                vdata[d].norm += faceNormals[--n4];

                perFace = 3;
            }
        }
        else if (line == "vn")
        {
            precomputedNormals = true;

            ss >> x >> y >> z;

            v.setX(x); v.setY(y); v.setZ(z);
            faceNormals.push_back(v);
        }
        else if (line == "vt")
        {
            texturePresent = true;
        }
        else if (line == "o")
        {
            precomputedNormals = false;
            texturePresent = false;
        }

        //mdl->progress->setValue(in.tellg());
        if (in.tellg() * 100 / size > last_progress * 100 / size)
        {
            last_progress = in.tellg();
            setProgress(last_progress);
        }

        if (cancelled)
            return;
    }

    if (precomputedNormals)
    {
        for (int i = 0; i < (int)vdata.size(); ++i)
            vdata[i].norm.normalize();
    }

    mdl->pivot /= vdata.size();
    mdl->bufSize = (int)indices.size();

    emit resultReady(std::move(vdata), std::move(indices));

    /*mdl->arrayBuf.bind();
    mdl->arrayBuf.allocate(vdata.data(), (int)vdata.size() * sizeof(Vertex));

    mdl->indexBuf.bind();
    mdl->indexBuf.allocate(indices.data(), (int)indices.size() * sizeof(GLuint));*/
}
