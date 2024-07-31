#include "FbxImporter.h"
#include "../ExternalCode/OpenFBX/src/ofbx.h"
#include <stdio.h>
#include "gl/glew.h"
#include <vector>
#include <string>

bool FbxImporter::ImportFBX(const char* filepath, std::vector<GLfloat>& outVertices, std::vector<GLuint>& outTriangles, std::string& diffuseTexturePath)
{
    FILE* fp;
    fopen_s(&fp, filepath, "rb");

    if (!fp) return false;

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    auto* content = new ofbx::u8[file_size];
    fread(content, 1, file_size, fp);

    ofbx::LoadFlags flags =
        //		ofbx::LoadFlags::IGNORE_MODELS |
        ofbx::LoadFlags::IGNORE_BLEND_SHAPES |
        ofbx::LoadFlags::IGNORE_CAMERAS |
        ofbx::LoadFlags::IGNORE_LIGHTS |
        //ofbx::LoadFlags::IGNORE_TEXTURES |
        //ofbx::LoadFlags::IGNORE_SKIN |
        //ofbx::LoadFlags::IGNORE_BONES |
        ofbx::LoadFlags::IGNORE_PIVOTS |
        //ofbx::LoadFlags::IGNORE_MATERIALS |
        ofbx::LoadFlags::IGNORE_POSES |
        ofbx::LoadFlags::IGNORE_VIDEOS |
        ofbx::LoadFlags::IGNORE_LIMBS; //|
        //		ofbx::LoadFlags::IGNORE_MESHES |
        //ofbx::LoadFlags::IGNORE_ANIMATIONS;

    ofbx::IScene* g_scene = ofbx::load((ofbx::u8*)content, file_size, (ofbx::u16)flags);

    delete[] content;
    fclose(fp);

    int mesh_count = g_scene->getMeshCount();
    int polygonCount = 0;
    int trianglesCount = 0;

    // output unindexed geometry
    for (int mesh_idx = 0; mesh_idx < mesh_count; ++mesh_idx) {
        const ofbx::Mesh& mesh = *g_scene->getMesh(mesh_idx);
        const ofbx::GeometryData& geom = mesh.getGeometryData();
        const ofbx::Skin* skin = mesh.getSkin();
        const ofbx::Cluster* cluster = skin->getCluster(0);
        
        for (int ab = 0; ab < skin->getClusterCount(); ab++)
        {
            const int* indices = skin->getCluster(ab)->getIndices();
            for (int cd = 0; cd < skin->getCluster(ab)->getIndicesCount(); cd++)
                printf("%d\n", indices[cd]);
        }

        // each ofbx::Mesh can have several materials == partitions
        for (int partition_idx = 0; partition_idx < geom.getPartitionCount(); ++partition_idx) {
            const ofbx::GeometryPartition& partition = geom.getPartition(partition_idx);

            for (int polygon_idx = 0; polygon_idx < partition.polygon_count; ++polygon_idx) {
                const ofbx::GeometryPartition::Polygon& polygon = partition.polygons[polygon_idx];

                for (int i = polygon.from_vertex; i < polygon.from_vertex + polygon.vertex_count; ++i) {
                    polygonCount++;
                }

                if (polygon.vertex_count == 3)
                    trianglesCount++;
                else if (polygon.vertex_count == 4)
                    trianglesCount += 2;
                else
                    trianglesCount += polygon.vertex_count - 2;
            }
        }
    }

    outVertices.resize(polygonCount * 8);
    outTriangles.resize(trianglesCount * 3);

    int currentOutVerticesIndex = 0;
    int currentOutTrianglesIndex = 0;

    for (int mesh_idx = 0; mesh_idx < mesh_count; ++mesh_idx) {
        const ofbx::Mesh& mesh = *g_scene->getMesh(mesh_idx);
        const ofbx::GeometryData& geom = mesh.getGeometryData();
        const ofbx::Vec3Attributes& positions = geom.getPositions();
        const ofbx::Vec3Attributes& normals = geom.getNormals();
        const ofbx::Vec2Attributes& uvs = geom.getUVs();

        for (int partition_idx = 0; partition_idx < geom.getPartitionCount(); ++partition_idx) {
            const ofbx::GeometryPartition& partition = geom.getPartition(partition_idx);

            for (int polygon_idx = 0; polygon_idx < partition.polygon_count; ++polygon_idx) {
                const ofbx::GeometryPartition::Polygon& polygon = partition.polygons[polygon_idx];

                for (int i = polygon.from_vertex; i < polygon.from_vertex + polygon.vertex_count; ++i) {
                    ofbx::Vec3 v = positions.get(i);
                    ofbx::Vec3 n = normals.values == nullptr ? ofbx::Vec3() : normals.get(i);
                    ofbx::Vec2 uv = uvs.values == nullptr ? ofbx::Vec2() : uvs.get(i);
                    outVertices[currentOutVerticesIndex + 0] = v.x;
                    outVertices[currentOutVerticesIndex + 1] = v.y;
                    outVertices[currentOutVerticesIndex + 2] = v.z;
                    outVertices[currentOutVerticesIndex + 3] = n.x;
                    outVertices[currentOutVerticesIndex + 4] = n.y;
                    outVertices[currentOutVerticesIndex + 5] = n.z;
                    outVertices[currentOutVerticesIndex + 6] = uv.x;
                    outVertices[currentOutVerticesIndex + 7] = uv.y;
                    currentOutVerticesIndex += 8;
                }

                if (polygon.vertex_count == 3) {
                    outTriangles[currentOutTrianglesIndex + 0] = polygon.from_vertex;
                    outTriangles[currentOutTrianglesIndex + 1] = polygon.from_vertex + 1;
                    outTriangles[currentOutTrianglesIndex + 2] = polygon.from_vertex + 2;
                    currentOutTrianglesIndex += 3;
                }
                else if (polygon.vertex_count == 4) {

                    outTriangles[currentOutTrianglesIndex + 0] = polygon.from_vertex;
                    outTriangles[currentOutTrianglesIndex + 1] = polygon.from_vertex + 1;
                    outTriangles[currentOutTrianglesIndex + 2] = polygon.from_vertex + 2;

                    outTriangles[currentOutTrianglesIndex + 3] = polygon.from_vertex;
                    outTriangles[currentOutTrianglesIndex + 4] = polygon.from_vertex + 2;
                    outTriangles[currentOutTrianglesIndex + 5] = polygon.from_vertex + 3;

                    currentOutTrianglesIndex += 6;
                }
                else {
                    for (int tri = 0; tri < polygon.vertex_count - 2; ++tri) {
                        outTriangles[currentOutTrianglesIndex + tri * 3 + 0] = polygon.from_vertex;
                        outTriangles[currentOutTrianglesIndex + tri * 3 + 1] = polygon.from_vertex + 1 + tri;
                        outTriangles[currentOutTrianglesIndex + tri * 3 + 2] = polygon.from_vertex + 2 + tri;
                    }
                    currentOutTrianglesIndex += 3 * (polygon.vertex_count - 2);
                }
            }
        }
    }

    return true;
}
