#include "model.hpp"
#include <tiny_obj_loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

Model::Model() {}
Model::~Model(){ destroy(); }

bool Model::init(const std::string &objPath){
    destroy();
    if(!loadObj(objPath)) return false;

    // create GPU buffers
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ibo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size()*sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size()*sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);

    // layout: position@0, normal@1, uv@2
    GLsizei stride = sizeof(Vertex);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(Vertex,pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(Vertex,normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,(void*)offsetof(Vertex,uv));

    glBindVertexArray(0);

    indexCount_ = indices_.size();

    // free CPU-side vectors if you want (keeps memory small)
    vertices_.clear(); vertices_.shrink_to_fit();
    indices_.clear(); indices_.shrink_to_fit();

    return true;
}

void Model::destroy(){
    if(ibo_){ glDeleteBuffers(1,&ibo_); ibo_=0; }
    if(vbo_){ glDeleteBuffers(1,&vbo_); vbo_=0; }
    if(vao_){ glDeleteVertexArrays(1,&vao_); vao_=0; }
    for(auto &m: materials_){ if(m.texID) glDeleteTextures(1, &m.texID); }
    materials_.clear();
    program_ = 0;
    modelMat_ = glm::mat4(1.0f);
}

void Model::translate(const glm::vec3 &t){ modelMat_ = glm::translate(modelMat_, t); }
void Model::rotate(float angleRadians, const glm::vec3 &axis){ modelMat_ = glm::rotate(modelMat_, angleRadians, axis); }
void Model::scale(const glm::vec3 &s){ modelMat_ = glm::scale(modelMat_, s); }
void Model::setProgram(GLuint program){
    program_ = program;
    ensureProgramUniforms();
}
void Model::setColor(const glm::vec3 &color){
    if(materials_.empty()) materials_.push_back({0,0,0,color,false});
    else materials_[0].color = color;
}

void Model::ensureProgramUniforms(){
    if(!program_) return;
    loc_MVP_ = glGetUniformLocation(program_, "MVP");
    loc_model_ = glGetUniformLocation(program_, "model");
    loc_uAlbedo_ = glGetUniformLocation(program_, "uAlbedo");
    loc_uUseTex_ = glGetUniformLocation(program_, "uUseTex");
    loc_uColor_ = glGetUniformLocation(program_, "uColor");
    loc_uLightDir_ = glGetUniformLocation(program_, "uLightDir");
    loc_uAmbient_ = glGetUniformLocation(program_, "uAmbient");
    // bind sampler to unit 0 once
    glUseProgram(program_);
    if(loc_uAlbedo_>=0) glUniform1i(loc_uAlbedo_, 0);
    glUseProgram(0);
}

void Model::render(const glm::mat4 &projection, const glm::mat4 &view){
    if(!valid() || program_==0) return;
    glm::mat4 MVP = projection * view * modelMat_;
    glUseProgram(program_);
    if(loc_MVP_>=0) glUniformMatrix4fv(loc_MVP_, 1, GL_FALSE, glm::value_ptr(MVP));
    if(loc_model_>=0) glUniformMatrix4fv(loc_model_,1,GL_FALSE,glm::value_ptr(modelMat_));

    // default light/uniforms (можно менять извне)
    if(loc_uLightDir_>=0) glUniform3f(loc_uLightDir_, 0.5f, -1.0f, 0.3f);
    if(loc_uAmbient_>=0) glUniform3f(loc_uAmbient_, 0.12f,0.12f,0.12f);

    glBindVertexArray(vao_);
    // если несколько материалов — отрисовываем диапазонами
    if(materials_.empty()){
        glDrawElements(GL_TRIANGLES, (GLsizei)indexCount_, GL_UNSIGNED_INT, 0);
    } else {
        size_t offset = 0;
        for(const auto &m : materials_){
            if(m.useTex && m.texID){
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m.texID);
                if(loc_uUseTex_>=0) glUniform1i(loc_uUseTex_, 1);
            } else {
                if(loc_uUseTex_>=0) glUniform1i(loc_uUseTex_, 0);
                if(loc_uColor_>=0) glUniform3fv(loc_uColor_, 1, glm::value_ptr(m.color));
            }
            glDrawElements(GL_TRIANGLES, (GLsizei)m.count, GL_UNSIGNED_INT, (void*)(offset * sizeof(unsigned int)));
            offset += m.count;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glBindVertexArray(0);
    glUseProgram(0);
}

bool Model::loadObj(const std::string &path){
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> mats;
    std::string warn, err;
    fs::path p(path);
    fs::path base = p.parent_path();

    if(!tinyobj::LoadObj(&attrib, &shapes, &mats, &warn, &err, path.c_str())){
        std::cerr << "tinyobj load error: " << warn << " " << err << "\n";
        return false;
    }

    // map material name -> texture id and ranges
    materials_.clear();
    struct Key{ int vi, ni, ti; };
    struct KeyHash{ size_t operator()(Key const&k) const noexcept { return (k.vi*73856093u) ^ (k.ni*19349663u) ^ (k.ti*83492791u); } };
    struct KeyEq{ bool operator()(Key const&a, Key const&b) const noexcept { return a.vi==b.vi && a.ni==b.ni && a.ti==b.ti; } };

    std::unordered_map<Key, unsigned int, KeyHash, KeyEq> vertCache;
    std::unordered_map<int, size_t> matStartIndex; // material id -> start index offset in indices_
    std::vector<int> faceMat; // per face material id

    // Preprocess faces' material ids
    for(const auto &shape : shapes){
        for(size_t f=0; f<shape.mesh.material_ids.size(); ++f) faceMat.push_back(shape.mesh.material_ids[f]);
    }

    // iterate shapes and faces, build unique vertices + indices and material ranges
    size_t globalFaceIdx = 0;
    for(const auto &shape : shapes){
        size_t idx_off = 0;
        for(size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f){
            int fv = shape.mesh.num_face_vertices[f];
            int matId = shape.mesh.material_ids[f]; // could be -1
            if(matStartIndex.find(matId) == matStartIndex.end()){
                matStartIndex[matId] = indices_.size();
                // create new material record
                MatRange mr;
                mr.texID = 0; mr.start = 0; mr.count = 0; mr.color = glm::vec3(0.8f,0.8f,0.8f); mr.useTex = false;
                // if matId valid, set color and texture path
                if(matId >= 0 && matId < (int)mats.size()){
                    auto &mt = mats[matId];
                    mr.color = glm::vec3(mt.diffuse[0], mt.diffuse[1], mt.diffuse[2]);
                    if(!mt.diffuse_texname.empty()){
                        fs::path texPath = base / mt.diffuse_texname;
                        if(fs::exists(texPath)){
                            mr.texID = loadTextureFromFile(texPath.string());
                            mr.useTex = (mr.texID != 0);
                        } else {
                            // try relative without base
                            if(fs::exists(mt.diffuse_texname)){
                                mr.texID = loadTextureFromFile(mt.diffuse_texname);
                                mr.useTex = (mr.texID != 0);
                            } else {
                                std::cerr << "Texture not found: " << mt.diffuse_texname << "\n";
                            }
                        }
                    }
                }
                materials_.push_back(mr);
            }

            for(int v = 0; v < fv; ++v){
                tinyobj::index_t idx = shape.mesh.indices[idx_off + v];
                Key key{ idx.vertex_index, idx.normal_index, idx.texcoord_index };
                auto it = vertCache.find(key);
                unsigned int vi;
                if(it != vertCache.end()){
                    vi = it->second;
                } else {
                    Vertex vert{};
                    if(key.vi >= 0){
                        vert.pos = {
                            attrib.vertices[3*key.vi+0],
                            attrib.vertices[3*key.vi+1],
                            attrib.vertices[3*key.vi+2]
                        };
                    }
                    if(key.ni >= 0){
                        vert.normal = {
                            attrib.normals[3*key.ni+0],
                            attrib.normals[3*key.ni+1],
                            attrib.normals[3*key.ni+2]
                        };
                    } else vert.normal = glm::vec3(0.0f);
                    if(key.ti >= 0){
                        vert.uv = {
                            attrib.texcoords[2*key.ti+0],
                            attrib.texcoords[2*key.ti+1]
                        };
                    } else vert.uv = glm::vec2(0.0f,0.0f);

                    vi = (unsigned int)vertices_.size();
                    vertices_.push_back(vert);
                    vertCache.emplace(key, vi);
                }
                indices_.push_back(vi);
            }
            idx_off += fv;
            ++globalFaceIdx;
        }
    }

    // finalize material ranges: count = next_start - start
    // matStartIndex keyed by matId but materials_ vector order corresponds to first-seen matId insertion order.
    // Build ordered ranges by iterating matStartIndex in insertion order via materials_ vector:
    // We stored materials_ in insertion order; but need to compute counts by scanning indices_ and faces again.
    // Simpler: compute counts by mapping matId->count
    std::unordered_map<int, size_t> matCounts;
    size_t idx_off2 = 0;
    for(const auto &shape : shapes){
        for(size_t f=0; f<shape.mesh.num_face_vertices.size(); ++f){
            int fv = shape.mesh.num_face_vertices[f];
            int matId = shape.mesh.material_ids[f];
            matCounts[matId] += fv;
            idx_off2 += fv;
        }
    }
    // Now rebuild materials_ in same insertion order as matStartIndex (which used map find earlier).
    // To keep it simple, clear and rebuild in order of matCounts keys:
    std::vector<MatRange> rebuilt;
    rebuilt.reserve(matCounts.size());
    for(auto &p : matStartIndex){
        int matId = p.first;
        MatRange mr = {};
        mr.start = p.second;
        mr.count = matCounts[matId];
        mr.useTex = false; mr.texID = 0; mr.color = glm::vec3(0.8f);
        if(matId >=0 && matId < (int)mats.size()){
            auto &mt = mats[matId];
            mr.color = glm::vec3(mt.diffuse[0], mt.diffuse[1], mt.diffuse[2]);
            if(!mt.diffuse_texname.empty()){
                fs::path texPath = base / mt.diffuse_texname;
                if(fs::exists(texPath)){
                    mr.texID = loadTextureFromFile(texPath.string());
                    mr.useTex = (mr.texID != 0);
                }
            }
        }
        rebuilt.push_back(mr);
    }
    // if no materials discovered, create a default single range covering all
    if(rebuilt.empty()){
        MatRange mr; mr.start = 0; mr.count = indices_.size(); mr.texID = 0; mr.useTex = false; mr.color = glm::vec3(0.8f);
        rebuilt.push_back(mr);
    }
    materials_.swap(rebuilt);

    // If normals are missing, generate simple per-triangle normals
    bool hasNormals = false;
    for(const auto &v : vertices_) if(glm::length(v.normal) > 0.0f){ hasNormals = true; break; }
    if(!hasNormals){
        for(size_t i=0;i+2<indices_.size();i+=3){
            Vertex &a = vertices_[indices_[i+0]];
            Vertex &b = vertices_[indices_[i+1]];
            Vertex &c = vertices_[indices_[i+2]];
            glm::vec3 n = glm::cross(b.pos - a.pos, c.pos - a.pos);
            a.normal += n; b.normal += n; c.normal += n;
        }
        for(auto &v : vertices_) v.normal = glm::normalize(v.normal);
    }

    return true;
}

GLuint Model::loadTextureFromFile(const std::string &path){
    int w,h,channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if(!data){
        std::cerr << "Failed to load texture: " << path << "\n";
        return 0;
    }
    GLuint tex=0;
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D,0);
    stbi_image_free(data);
    return tex;
}
