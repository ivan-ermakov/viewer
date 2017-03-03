#include "model.h"

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <locale>

#include <QVector2D>
#include <QVector3D>

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

struct Vertex
{
    QVector3D pos;
    QVector3D norm;
};

Model::Model()
    : indexBuf(QOpenGLBuffer::IndexBuffer)
{
    initializeOpenGLFunctions();

    arrayBuf.create();
    indexBuf.create();
}

Model::Model(std::string file)
    : Model()
{
    loadObj(file);
}

Model::~Model()
{
    arrayBuf.destroy();
    indexBuf.destroy();
}

void Model::loadObj(std::string filename)
{
    std::vector<QVector3D> faceNormals;
    std::vector<Vertex> vdata;
    std::vector<GLuint> indices;
    pivot = QVector3D(0, 0, 0);

    std::ifstream in(filename, std::ios::in);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << std::endl;
        return;
    }

    bool precomputedNormals = false;
    bool texturePresent = false;
    //int perFace = 3;
    //char chr;
    Vertex vtx;
    QVector3D v;
    float x, y, z;
    GLuint a, b, c, d, n, n2, n3, n4;
    std::string line;
    std::string s;
    std::string str;
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

            pivot += vtx.pos;
        }
        else if (line == "f")
        {
             ss >> a;
             if (texturePresent) ss >> n;
             if (precomputedNormals) ss >> n;

             //std::cout << s << "\t" << a << "/" << n << "\t";

             ss >> b;
             if (texturePresent) ss >> n2;
             if (precomputedNormals) ss >> n2;
             //std::cout << b << "/" << n2 << "\t";

             ss >> c;
             if (texturePresent) ss >> n3;
             if (precomputedNormals) ss >> n3;
             //std::cout << c << "/" << n3 << "\n";

             ss >> d;
             if (texturePresent) ss >> n4;
             if (precomputedNormals) ss >> n4;

             indices.push_back(--a);
             indices.push_back(--b);
             indices.push_back(--c);

             vdata[a].norm += faceNormals[--n];
             vdata[b].norm += faceNormals[--n2];
             vdata[c].norm += faceNormals[--n3];

            if (!ss.fail())
            {
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(--d);

                vdata[d].norm += faceNormals[--n4];
            }
        }
        else if (line == "vn")
        {
            if (!precomputedNormals)
                precomputedNormals = true;

            ss >> x >> y >> z;

            v.setX(x); v.setY(y); v.setZ(z);
            faceNormals.push_back(v);
        }
        else if (line == "vt")
        {
            if (!texturePresent)
                texturePresent = true;
        }
    }

    if (precomputedNormals)
    {
        for (int i = 0; i < (int)vdata.size(); ++i)
            vdata[i].norm.normalize();
    }

    pivot /= vdata.size();

    arrayBuf.bind();
    arrayBuf.allocate(vdata.data(), (int)vdata.size() * sizeof(Vertex));

    indexBuf.bind();
    indexBuf.allocate(indices.data(), (int)indices.size() * sizeof(GLuint));

    bufSize = (int)indices.size();
}

void Model::draw(QOpenGLShaderProgram *program)
{
    // Tell OpenGL which VBOs to use
    arrayBuf.bind();
    indexBuf.bind();

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("vPos");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(Vertex));

    int normalLocation = program->attributeLocation("vNormal");
    program->enableAttributeArray(normalLocation);
    program->setAttributeBuffer(normalLocation, GL_FLOAT, sizeof(QVector3D), 3, sizeof(Vertex));

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    /*int texcoordLocation = program->attributeLocation("a_texcoord");
    program->enableAttributeArray(texcoordLocation);
    program->setAttributeBuffer(texcoordLocation, GL_FLOAT, 0, 2, sizeof(QVector3D));*/

    // Draw cube geometry using indices from VBO 1
    glDrawElements(GL_TRIANGLES, bufSize, GL_UNSIGNED_INT, 0);
}
