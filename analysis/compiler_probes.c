/* Reconstructed behavioral probes, not recovered original source. */
void probe_translate(float x, float y, float z, float *m)
{
    m[3] += x;
    m[7] += y;
    m[11] += z;
}

void probe_cross(float *a, float *b, float *out)
{
    out[0] = a[1] * b[2] - b[1] * a[2];
    out[1] = a[2] * b[0] - b[2] * a[0];
    out[2] = b[1] * a[0] - a[1] * b[0];
}
