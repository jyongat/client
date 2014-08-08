/**
 * Created with JetBrains WebStorm.
 * User: Administrator
 * Date: 12-10-26
 * Time: ����3:44
 * To change this template use File | Settings | File Templates.
 */

var net = require('net');
var timeout = 20000;//��ʱ
var listenPort = 3001;//�����˿�

var server = net.createServer(function(socket){
    // ���ǻ��һ������ - �������Զ�����һ��socket����
    console.log('connect: ' +
        socket.remoteAddress + ':' + socket.remotePort);
    //socket.setEncoding('binary');
    //��ʱ�¼�
    socket.setTimeout(timeout,function(){
        console.log('���ӳ�ʱ');
        socket.end();
    });

    //���յ�����
    socket.on('data',function(data){

       console.log('recv:' + data);
       socket.write(data);

    });

    //���ݴ����¼�
    socket.on('error',function(exception){
        console.log('socket error:' + exception);
        socket.end();
    });
    //�ͻ��˹ر��¼�
    socket.on('close',function(data){
        console.log('close: ' +
            socket.remoteAddress + ' ' + socket.remotePort);

    });


}).listen(listenPort);

//�����������¼�
server.on('listening',function(){
    console.log("server listening:" + server.address().port);
});

//�����������¼�
server.on("error",function(exception){
    console.log("server error:" + exception);
}); 