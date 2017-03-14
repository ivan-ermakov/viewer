#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <locale>

#include "debug/Stable.h"

#include <QMutexLocker>

#include "model.h"
#include "modelloader.h"

// Treat / as whitespace
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
        return rc;
    }
};

ModelLoader::ModelLoader() :
    QThread(),
    progress(0),
    ready(false),
    cancelled(false),
    mtx(nullptr)
{}

ModelLoader::ModelLoader(QString fname) :
    QThread(),
    progress(0),
    ready(false),
    cancelled(false),
    fileName(fname),
    mtx(new QMutex())
{}

ModelLoader::~ModelLoader()
{
    if (mtx)
        delete mtx;
}

bool ModelLoader::isCancelled()
{
    return cancelled;
}

bool ModelLoader::isReady()
{
    return ready;
}

int ModelLoader::getProgress()
{
    QMutexLocker lck(mtx);
    return progress;
}

int ModelLoader::getMaxProgress()
{
    QMutexLocker lck(mtx);
    return maxProgress;
}

const std::vector<Vertex>& ModelLoader::getVertices()
{
    return vertices;
}

const std::vector<GLuint>& ModelLoader::getIndices()
{
    return indices;
}

QVector3D ModelLoader::getPivot()
{
    return pivot;
}

void ModelLoader::cancel()
{
    cancelled = true;
}

void ModelLoader::read(Model* mdl)
{
    if (ready)
    {
        QMutexLocker lck(&(mdl->mutex));
        mdl->load(vertices, indices, pivot);
    }
}

void ModelLoader::run()
{
    std::vector<QVector3D> faceNormals;
    pivot = QVector3D(0, 0, 0);

    std::ifstream in(fileName.toStdString(), std::ios::in);
    if (!in)
    {
        std::cerr << "Cannot open " << fileName.toStdString() << std::endl;
        return;
    }

    mtx->lock();
    progress = 0;

    in.seekg(0, std::ios::end);
    maxProgress = in.tellg();
    in.seekg(0, std::ios::beg);
    mtx->unlock();

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

#undef new
    Ctype* ct = new Ctype();
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    ss.imbue(std::locale(std::locale(), ct));

    while (std::getline(in, s))
    {
        ss.clear();
        ss.str(s);
        ss >> line;

        if (line == "v")
        {
            ss >> x >> y >> z;

            vtx.pos.setX(x); vtx.pos.setY(y); vtx.pos.setZ(z);
            vertices.push_back(vtx);

            pivot += vtx.pos;
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

             if (a < 0)
                 a = vertices.size() + a;
             else
                 --a;

             ss >> b;
             if (texturePresent) ss >> n2;
             if (precomputedNormals) ss >> n2;

             if (ss.fail())
             {
                 std::cerr << "ERROR: invalid model file.\n";
                 return;
             }

             if (b < 0)
                 b = vertices.size() + b;
             else
                 --b;

             ss >> c;
             if (texturePresent) ss >> n3;
             if (precomputedNormals) ss >> n3;

             if (c < 0)
                 c = vertices.size() + c;
             else
                 --c;

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

                 if (d < 0)
                     d = vertices.size() + d;
                 else
                     --d;
             }

             indices.push_back(a);
             indices.push_back(b);
             indices.push_back(c);

             vertices[a].norm += faceNormals[--n];
             vertices[b].norm += faceNormals[--n2];
             vertices[c].norm += faceNormals[--n3];

            if (perFace == 4)
            {
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(d);

                vertices[d].norm += faceNormals[--n4];

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
        /*else
            std::cout << "Unknown line: " << s << "\n";*/

        mtx->lock();
        progress = in.tellg();
        mtx->unlock();

        if (cancelled)
            return;
    }

    if (precomputedNormals)
    {
        for (int i = 0; i < (int)vertices.size(); ++i)
            vertices[i].norm.normalize();
    }

    pivot /= vertices.size();

    ready = true;
}
