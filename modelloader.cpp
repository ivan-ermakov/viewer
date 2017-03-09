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
    mdl(nullptr)
{}

ModelLoader::ModelLoader(Model* mdl_, QString fname) :
    QThread(),
    progress(0),
    ready(false),
    cancelled(false),
    fileName(fname),
    mdl(mdl_),
    mtx(new QMutex())
{}

ModelLoader::~ModelLoader()
{}

bool ModelLoader::isReady() const
{
    return ready;
}

int ModelLoader::getProgress() const
{
    QMutexLocker lck(mtx);
    return progress;
}

int ModelLoader::getMaxProgress() const
{
    QMutexLocker lck(mtx);
    return maxProgress;
}

void ModelLoader::cancel()
{
    cancelled = true;
}

void ModelLoader::read()
{
    if (ready)
    {
        //mdl->load(std::move(vdata), std::move(indices));

        mdl->arrayBuf.bind();
        mdl->arrayBuf.allocate(vdata.data(), (int)vdata.size() * sizeof(Vertex));

        mdl->indexBuf.bind();
        mdl->indexBuf.allocate(indices.data(), (int)indices.size() * sizeof(GLuint));

        mdl->bufSize = indices.size();
    }
}

void ModelLoader::run()
{
    QMutexLocker lck(&(mdl->mutex));
    std::vector<QVector3D> faceNormals;
    mdl->pivot = QVector3D(0, 0, 0);

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

    //setMaxProgress(maxProgress);
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

             /*if (a < 0)
                 a = vdata.size() + a;
             else*/
                 --a;

             //std::cout << s << "\t" << a << "/" << n << "\t";

             ss >> b;
             if (texturePresent) ss >> n2;
             if (precomputedNormals) ss >> n2;

             if (ss.fail())
             {
                 std::cerr << "ERROR: invalid model file.\n";
                 return;
             }

             /*if (b < 0)
                 b = vdata.size() + b;
             else*/
                 --b;

             //std::cout << b << "/" << n2 << "\t";

             ss >> c;
             if (texturePresent) ss >> n3;
             if (precomputedNormals) ss >> n3;

             /*if (c < 0)
                 c = vdata.size() + c;
             else*/
                 --c;

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

                 /*if (d < 0)
                     d = vdata.size() + d;
                 else*/
                     --d;
             }

             indices.push_back(a);
             indices.push_back(b);
             indices.push_back(c);

             vdata[a].norm += faceNormals[--n];
             vdata[b].norm += faceNormals[--n2];
             vdata[c].norm += faceNormals[--n3];

            if (perFace == 4)
            {
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(d);

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
        /*else
            std::cout << "Unknown line: " << s << "\n";*/

        //mdl->progress->setValue(in.tellg());
        mtx->lock();
        progress = in.tellg();
        mtx->unlock();

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

    ready = true;

    //emit resultReady(std::move(vdata), std::move(indices));
}
