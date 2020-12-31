using System;
using System.Runtime.InteropServices;
using UnityEngine;

public class MsgConverter : MonoBehaviour
{
    /// <summary>
        /// 将结构转换为字节数组
        /// </summary>
        /// <param name="obj">结构对象</param>
        /// <returns>字节数组</returns>
    public static byte[] StructToBytes(object obj)
    {
        Int32 size = Marshal.SizeOf(obj);
        Console.WriteLine(size);
        IntPtr buffer = Marshal.AllocHGlobal(size);
        try
        {
            Marshal.StructureToPtr(obj, buffer, false);
            Byte[] bytes = new Byte[size];
            Marshal.Copy(buffer, bytes, 0, size);
            return bytes;
        }
        finally
        {
            Marshal.FreeHGlobal(buffer);
        }
    }
    public static object BytesToStruct(byte[] bytes, Type type)
    {
        //得到结构的大小
        int size = Marshal.SizeOf(type);

        //byte数组长度小于结构的大小
        if (size > bytes.Length)
        {
            //返回空
            return null;
        }
        //分配结构大小的内存空间
        IntPtr structPtr = Marshal.AllocHGlobal(size);
        //将byte数组拷到分配好的内存空间
        Marshal.Copy(bytes, 0, structPtr, size);
        //将内存空间转换为目标结构
        object obj = Marshal.PtrToStructure(structPtr, type);
        //释放内存空间
        Marshal.FreeHGlobal(structPtr);
        //返回结构
        return obj;
    }
}
