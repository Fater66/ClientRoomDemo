using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

// c#结构转换
// https://www.cnblogs.com/rainbow70626/p/8000984.html

//[Serializable] //序列化对象
//[StructLayout(LayoutKind.Sequential, Pack = 1)] // 按1字节对齐
//public class UserMsg
//{
//    public int messageID;
//    public int clientID;
//    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 200)] //限制200字节
//    public byte[] message;
//};

[Serializable] //序列化对象
[StructLayout(LayoutKind.Sequential, Pack = 1)] // 按1字节对齐
public class Packet
{
    public bool isSetUserId;
    public int userId;
    public int id;
    public int len;
    public bool isChangeRoom;
    public int roomId;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
    public string name;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 100)] //限制100字节
    public string content;

    public Packet(int userId,int id,bool isChangeRoom,bool isSetUserId, int roomId, string user, string content) // 初始化
    {
        this.userId = userId;
        this.id = id;
        this.len = content.Length;
        this.isChangeRoom = isChangeRoom;
        this.isSetUserId = isSetUserId;
        this.roomId = roomId;
        this.name = user.PadRight(16, '\0');
        this.content = content.PadRight(100, '\0');
    }
}