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


    string contentToShow = "";
    int curId = 0;
    int curRoomId = -1;
    int curUserId;

    TcpClient socket;
    Thread thread;
    Thread recvThread;
    NetworkStream ns;
    

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

    public void SendMsgPacket()
    {
        SendPacket(0, curRoomId, nameInput.text, contentInput.text);

    }
    public void SendPacket(int mode,int roomId,string packName,string packContent)
    {
        // 发送名字

        Packet pack = new Packet(curUserId,curId,mode,roomId,packName, packContent);
        curId++;

        Debug.Log("发送消息:" + ",房间号:" + curRoomId + ",昵称：" + packName + ",内容:"+pack.content);
        byte[] message = MsgConverter.StructToBytes(pack);

        Debug.Log("message len:" + message.Length);
        ns.Write(message, 0, message.Length);
    }

    void ReceivePacket()
    {
        if(ns != null)
        {
            Packet pack = new Packet(curUserId, -1,0,curRoomId,"", "");
            while (true)
            {
                byte[] readBuf = new byte[Marshal.SizeOf(pack)];
                ns.Read(readBuf, 0, readBuf.Length);
                pack = (Packet)MsgConverter.BytesToStruct(readBuf, pack.GetType());
                //Debug.Log("收到" + System.Text.Encoding.ASCII.GetString(readBuf));
                Debug.Log("收到消息," + ",房间号:" + curRoomId + ",昵称：" + pack.name + ",内容:" + pack.content + ",mode:" + pack.mode);
                contentToShow = pack.name + ":" + pack.content + "\n";
                if(pack.mode == 1)
                {
                    curUserId = pack.userId;
                    Debug.Log("收到服务器UID" + curUserId);
                }
                if(pack.mode == 5)
                {
                    curRoomId = -1;
                }
            }
        }
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
    }


    private void Start()
    {
        curUserId = (int)Random.Range(1f, 10000f);
        Debug.Log("UserId初始化为"+ curUserId);
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
    }

    private void OnApplicationQuit()
    {
        ns.Close();
    }
}


