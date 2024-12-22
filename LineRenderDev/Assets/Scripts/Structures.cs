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
    public ArgBufferLayout(bool EnableDebug)
    {
        uint VertexCount = 3;
        if (EnableDebug) VertexCount = 2;
        Buffer = new uint[4 * 6] {
            VertexCount, 0, 0, 0, // indirect draw : vertex count, instance count, start vertex, start instance
            1, 1, 1, 0, // contourize pass : contour count / thread count, 1, 1, contour count
            1, 1, 1, 0, // slice pass : visible contour count / thread count, 1, 1, visible contour count
            0, 1, 1, 0, // visiblity pass : slice count, 1, 1, 0
            1, 1, 1, 0, // generate pass : segment count / thread count, 1, 1, 0
            1, 1, 1, 0, // chainning pass : head count / thread count, 1, 1, head count
        };
    }
    public uint[] Buffer;
    public int IndirectDrawStart() { return 0; }
    public int IndirectDrawTriangleCount() { return IndirectDrawStart() + StrideSize() * 1; }

    public int ContourizePassStart() { return StrideSize() * 4; }
    public int ContourizePassDispatchCount() { return ContourizePassStart(); }
    public int ContourizePassContourCount() { return ContourizePassStart() + StrideSize() * 3; }

    public int SlicePassStart() { return StrideSize() * 8; }
    public int SlicePassDispatchCount() { return SlicePassStart(); }
    public int SlicePassContourCount() { return SlicePassStart() + StrideSize() * 3; }

    public int VisiblityPassStart() { return StrideSize() * 12; }
    public int VisiblityPassDispatchCount() { return VisiblityPassStart(); }
    public int VisiblityPassSliceCount() { return VisiblityPassStart(); }

    public int GeneratePassStart() { return StrideSize() * 16; }
    public int GeneratePassDispatchCount() { return GeneratePassStart(); }
    public int GeneratePassSegmentCount() { return GeneratePassStart() + StrideSize() * 3; }

    public int ChainningPassStart() { return StrideSize() * 20; }
    public int ChainningPassDispatchCount() { return ChainningPassStart(); }
    public int ChainningPassLineHeadCount() { return ChainningPassStart() + StrideSize() * 3; }


    public int StrideSize()
    {
        return sizeof(uint);
    }

    public int Count()
    {
        return 4 * 6;
    }
}



public struct Contour
{
    public static int Size()
    {
        return sizeof(uint) * 1 + sizeof(uint) * 3 + sizeof(float) * 3 * 3 + sizeof(uint) * 2 + sizeof(uint) * 2;
    }
}

public struct Segment
{
    public static int Size(bool EnableDebug)
    {
        return sizeof(float) * 2 * 2 + (EnableDebug ? sizeof(float) * 4 * 2 : 0) + sizeof(float) * 2 + sizeof(uint) * 1 + sizeof(uint) * 1 + sizeof(uint) * 1 + sizeof(uint) * 2 + sizeof(uint) * 2;
    }
}

/*
public struct SliceMetaData
{
    public static int Size(bool EnableDebug)
    {
        return sizeof(uint) * 5 + sizeof(float) * 2 + sizeof(float) * 1 + (EnableDebug ? sizeof(float) * 4 : 0);
    }
}*/
public struct Slice
{
    public static int Size()
    {
        return sizeof(uint) * 3;// + SliceMetaData.Size(EnableDebug);
    }
}

public struct SegmentMetaData
{
    public static int Size(bool EnableDebug)
    {
        return sizeof(float) * 4 * 2 + (EnableDebug ? sizeof(float) * 4 * 2 : 0) + sizeof(uint) * 3 + sizeof(uint) * 1 + sizeof(uint) * 1 + sizeof(float) * 1 + sizeof(uint) * 2 + sizeof(uint) * 2;
    }
}


public struct LineHead
{
    public static int Size()
    {
        return sizeof(uint) * 1;
    }
}


public struct AnchorVertex
{
    public static int Size()
    {
        return sizeof(uint) * 3 * 2;
    }
}
public struct AnchorEdge
{
    public static int Size()
    {
        return sizeof(uint) * 3;
    }
}
public struct AnchorSlice
{
    public static int Size()
    {
        return sizeof(uint) * 2;
    }
}

public struct LineMesh
{
    public static int Size(bool EnableDebug)
    {
        return sizeof(float) * 2 * 2 + (EnableDebug ? (sizeof(float) * 4 * 2 + sizeof(uint)) : 0) + sizeof(float) * 2 + sizeof(uint) * 1 + sizeof(uint) * 2 + sizeof(uint) * 2 + sizeof(uint) * 2 + sizeof(float) * 2;
    }
}
