static void section(unsigned char *data, unsigned long *at, unsigned long count, const void *p,
                    unsigned long size) {
    memcpy(data + *at, &count, 4);
    *at += 4;
    memcpy(data + *at, p, count * size);
    *at += count * size;
}
static unsigned long fixture(unsigned char *data, int destination) {
    BHBspMaterial material;
    BHBspVertex vertices[4];
    BHBspEdge edges[4];
    BHBspFace faces[3];
    BHBspNode nodes[3];
    BHBspLeaf leaves[3];
    unsigned long at = 0, i;
    memset(&material, 0, sizeof(material));
    strcpy(material.texture, "STONE.PCX");
    memset(vertices, 0, sizeof(vertices));
    memset(edges, 0, sizeof(edges));
    memset(faces, 0, sizeof(faces));
    memset(nodes, 0, sizeof(nodes));
    for (i = 0; i < 4; ++i) {
        vertices[i].y = (i == 0 || i == 3) ? -2 : 2;
        if (destination)
            vertices[i].z = i < 2 ? -1 : 1;
        else
            vertices[i].x = i < 2 ? -1 : 1;
        edges[i].vertex = i;
        edges[i].next = i == 3 ? -1 : i + 1;
    }
    for (i = 0; i < 3; ++i) {
        faces[i].first_edge = 0;
        nodes[i].minimum[0] = destination ? 0 : -1;
        nodes[i].maximum[0] = destination ? 0 : 1;
        nodes[i].minimum[1] = -2;
        nodes[i].maximum[1] = 2;
        nodes[i].minimum[2] = destination ? -1 : 0;
        nodes[i].maximum[2] = destination ? 1 : 0;
        nodes[i].normal[destination ? 0 : 2] = 1;
        nodes[i].front_leaf = i;
        nodes[i].back_leaf = -1;
        nodes[i].front_node = nodes[i].back_node = -1;
        leaves[i].face = i;
        leaves[i].next = -1;
    }
    section(data, &at, 1, &material, sizeof(material));
    section(data, &at, 4, vertices, sizeof(vertices[0]));
    section(data, &at, 4, edges, sizeof(edges[0]));
    section(data, &at, 3, faces, sizeof(faces[0]));
    section(data, &at, 3, nodes, sizeof(nodes[0]));
    section(data, &at, 3, leaves, sizeof(leaves[0]));
    return at;
}
