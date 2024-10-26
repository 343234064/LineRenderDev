using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public struct Array2<T>
{
    public Array2(T _x, T _y)
    {
        x = _x;
        y = _y;
    }

    public T x;
    public T y;

    public override string ToString() => $"({x}, {y})";
}


public struct Array3<T>
{
    public Array3(T _x, T _y, T _z)
    {
        x = _x;
        y = _y;
        z = _z;
    }

    public T x;
    public T y;
    public T z;

    public override string ToString() => $"({x}, {y}, {z})";
}

public struct Array4<T>
{
    public Array4(T _x, T _y, T _z, T _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }

    public T x;
    public T y;
    public T z;
    public T w;

    public override string ToString() => $"({x}, {y}, {z}, {w})";
}












public class ArgBufferLayout
{
    public ArgBufferLayout()
    {
        Buffer = new uint[8] {
            2, 0, 0, 0, // indirect draw : vertex count, triangle count, start vertex, start instance
            1, 1, 1, 0, // contourize pass : contour face count / thread count, 1, 1, contour face count
        };
    }
    public uint[] Buffer;
    public int IndirectDrawStart() { return 0; }
    public int IndirectDrawTriangleCount() { return IndirectDrawStart() + StrideSize() * 1; }
    public int ContourizePassStart() { return StrideSize() * 4; }
    public int ContourizePassDispatchCount() { return ContourizePassStart(); }
    public int ContourizePassFaceCount() { return ContourizePassStart() + StrideSize() * 3; }

    public int StrideSize()
    {
        return sizeof(uint);
    }

    public int Count()
    {
        return 4 + 4;
    }
}



public struct ContourFace
{
    public static int Size()
    {
        return sizeof(uint) * 3 + sizeof(float) * 3 * 3;
    }
}

public struct Segment
{
    public static int Size()
    {
        return sizeof(float) * 4 * 2 + sizeof(float) * 4;
    }
}