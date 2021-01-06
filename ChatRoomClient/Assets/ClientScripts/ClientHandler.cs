using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Threading;
using UnityEngine;
using UnityEngine.UI;

public class ClientHandler : MonoBehaviour
{
    //UI 显示
    public InputField contentInput;
    public InputField nameInput;
    public InputField roomInput;
    public InputField userIdInput;

    public Text msgText;
    public Text debugText;

    bool heartTest = false;

    string contentToShow = "";
    int curId = 0;
    int curRoomId = -1;
    int curUserId;

    TcpClient socket;
    Thread thread;
    Thread recvThread;
    NetworkStream ns;

    Timer heartBitSender;
    double heartTimeStamp;
    bool isCheckHeart;

    public void SetUpClient(string IP)
    {

        thread = new Thread(ConnectToServer);
        thread.Start();

    }

    public void ReceiveMsg()
    {
        recvThread = new Thread(ReceivePacket);
        recvThread.Start();
    }

    public void CloseConnect()
    {
        
    }


    public void QuitRoom()
    {
        SendPacket(2, -1, nameInput.text, "");
    }

    public void EnterRoom()
    {
        int changeRoomId = int.Parse(roomInput.text);
        curRoomId = changeRoomId;
        SendPacket(4, changeRoomId, nameInput.text, "");
    }

    public void SetUpRoom()
    {
        int changeRoomId = int.Parse(roomInput.text);
        curRoomId = changeRoomId;
        SendPacket(3, changeRoomId, nameInput.text, "");
    }

    public void ChangeHeartTest()
    {
        heartTest = !heartTest;
    }
    private void HeartBit(object state)
    {
        if(!heartTest)
        {
            if(socket.Connected)
            {
                SendPacket(7, curRoomId, nameInput.text, "");
            }
            TimeSpan ts = DateTime.Now - new DateTime(1970, 1, 1, 0, 0, 0, 0);
            if (heartTimeStamp == 0) heartTimeStamp = ts.TotalSeconds;

            // 当前时间和一次心跳发生前的时间的差值
            double interval = ts.TotalSeconds - heartTimeStamp;
            Debug.Log("当前时间和一次心跳发生前的时间的差值:" + interval);
            
            if (interval > 3)
            {
                if (isCheckHeart)
                {
                    isCheckHeart = false;
                    heartTimeStamp = ts.TotalSeconds;
                    Debug.Log("服务端收到心跳检测报文");
                }
                else
                {
                    if (socket.Connected)
                    {
                        Debug.Log("客户端网络问题，尝试重连");
                        socket.Close();
                        ReconnectServer();
                    }
                    else
                    {
                        Debug.Log("服务端关闭");
                    }
                }
            }
        }

    }

    public void SendMsgPacket()
    {
        SendPacket(0, curRoomId, nameInput.text, contentInput.text);
    }

    public void SendPacket(int mode,int roomId,string packName,string packContent)
    {
        // 发送名字

        Packet pack = new Packet(curUserId,curId,mode,roomId,packName, packContent);
        curId++;

        Debug.Log("发送消息:" + "id:" + curId + "mode:" + mode + ",房间号:" + curRoomId + ",昵称：" + packName + ",内容:" + pack.content);
        byte[] message = MsgConverter.StructToBytes(pack);

        //Debug.Log("message len:" + message.Length);
        ns.Write(message, 0, message.Length);
    }

    void ReceivePacket()
    {
        int pre_id = -1;
        if(ns != null)
        {
            Packet pack = new Packet(curUserId, -1,0,curRoomId,"", "");
            while (true)
            {
                byte[] readBuf = new byte[Marshal.SizeOf(pack)];
                ns.Read(readBuf, 0, readBuf.Length);
                pack = (Packet)MsgConverter.BytesToStruct(readBuf, pack.GetType());
                if(pre_id == pack.id)
                {
                    continue;
                }
                else
                {
                    pre_id = pack.id;
                }
                //Debug.Log("收到" + System.Text.Encoding.ASCII.GetString(readBuf));
                //Debug.Log("收到消息:" + "id:" +pack.id + "mode:" + pack.mode + ",房间号:" + pack.roomId + ",昵称：" + pack.name + ",内容:" + pack.content);s
                contentToShow = pack.name + ":" + pack.content + "\n";
                if (pack.mode == 1)
                {
                    curUserId = pack.userId;
                    Debug.Log("收到服务器UID" + curUserId);
                }
                if (pack.mode == 5)
                {
                    curRoomId = -1;
                    Debug.Log("收到确认退出房间报文");
                }
                if (pack.mode == 8)
                {
                    contentToShow = ""; // 心跳确认报文不需要显示
                    isCheckHeart = true;
                    heartTimeStamp = 0;
                }
                if(pack.mode == 10)
                {
                    curUserId = pack.userId;
                    nameInput.text = pack.name;
                }
            }
        }
    }

    private void ReconnectServer()
    {
        // 先记录原来的id
        int preUserId = curUserId;
        heartBitSender.Dispose();
        ConnectToServer();
        SendPacket(9, curRoomId, "", preUserId.ToString());
    }

    private void ConnectToServer()
    {
        int port = 8888;
        //IP = "149.129.53.86";
        //IPAddress ip = IPAddress.Parse(IP); //将IP地址字符串转换成IPAddress实例
        IPAddress ip = IPAddress.Loopback;


        socket = new TcpClient();
        socket.Connect(ip, port);
        ns = socket.GetStream();
        Debug.Log("socket已连接");
        heartBitSender = new Timer(HeartBit, 0, 0, 3000);
    }


    private void Start()
    {
        ConnectToServer();
        ReceiveMsg();
    }

    private void Update()
    {
        if(!contentToShow.Equals(""))
        {
            msgText.text += contentToShow;
            contentToShow = "";
        }
        //curUserId = int.Parse(userIdInput.text);
        debugText.text = "curRoomId:" + curRoomId + "\nuserId:" + curUserId;
        //Debug.Log("socket" + socket.Connected);
    }

    private void OnApplicationQuit()
    {
        ns.Close();
        heartBitSender.Dispose();
    }
    
}


