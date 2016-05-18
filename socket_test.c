
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <cutils/sockets.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#include "dlp_common.h"
#include "dlp_socket.h"

#define SOCKET_NAME 	"dlpuart-server"
#define SOCKET_PREFIX   "/dev/socket/"

#define CMD_POWER					0x01
#define CMD_MOUNTINT_MODE			0x02
#define CMD_VERTICAL_KS				0x03
#define CMD_FACTORY_RST				0x04
#define CMD_BACKLIGHT				0x05
#define CMD_KEY_EVENT				0x06
#define CMD_DLP_MENU				0x07
#define CMD_3D_MODE					0x08
#define CMD_DISPLAY_MODE			0x09
#define CMD_SET_R_DUTY              0x0A
#define CMD_SET_G_DUTY              0x0B
#define CMD_SET_B_DUTY              0x0C
#define CMD_SET_Y_DUTY              0x0D
#define CMD_GET_DUTY                0x0E


#define CMD_CW_ERROR				0x80
#define CMD_FAN_LOCK				0x81
#define CMD_LAMP_ERROR				0x82
#define CMD_TEMP_WARNNING			0x83
#define CMD_SENSOR_ERROR			0x84

#define SOCKET_HEAD1				0x5a
#define SOCKET_HEAD2				0x5b
#define SOCKET_HEAD_LEN				2
#define SOCKET_CMDID_LEN			1
#define SOCKET_NEGATIVE_BYTE        1


typedef enum {
	MM_DESKTOP_FRONT = 0,
	MM_CEILING_FRONT,
	MM_DESKTOP_REAR,
	MM_CEILING_REAR
}EN_MountingMode;

typedef enum {
	KEY_UP = 1,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_SELECT,
	KEY_RETURN,
	KEY_EXIT
}EN_KeyEvent;

typedef enum {
	E3D_OFF = 0,
	E3D_AUTO,
	E3D_LEFT_RIGHT,
	E3D_TOP_BOTTON,
	E3D_FRAME_PACKING
}EN_3dMode;

typedef enum {
	DM_DYNAMIC = 0,
	DM_STANDARD,
	DM_SOFT,
	DM_GORGEOUS
}EN_DisplayMode;


int socketFd = 0;

int socket_connect()
{
    unsigned int len, nameLen;
	struct sockaddr_un sockUn;
	int i = 0;
	
	if((socketFd = socket(AF_LOCAL,SOCK_STREAM,0)) < 0)
	{
        DLP_DBG("create socket() FAIL!\n");
    	return -1;
    }
	
	nameLen = strlen(SOCKET_NAME) + strlen(SOCKET_PREFIX);
    if (nameLen > sizeof(sockUn) - offsetof(struct sockaddr_un, sun_path) - 1) 
	{
        DLP_DBG("socket name len error!\n");
    	return -1;
    }

    strcpy(sockUn.sun_path, SOCKET_PREFIX);
    strcat(sockUn.sun_path, SOCKET_NAME);
    sockUn.sun_family = AF_LOCAL;
    len = nameLen + offsetof(struct sockaddr_un, sun_path) + 1;
	
	while (connect(socketFd, (struct sockaddr *)&sockUn, len) == -1)
    {
		i++;
		if (i >= 5)
		{
			//test for connect 5 times
			return -1;
		}
        sleep(1);
    }

    //register cmd for this socket: format->  head1| head2| 0x00| lenght(n)| cmd1| cmd2| cmd3|...| cmdn|
    char reg[] = {0x5a, 0x5b, 0x00, 0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
	if (send(socketFd, reg, sizeof(reg), 0) == -1)
	{
		DLP_DBG("socket register cmd FAIL!\n");
	}
	return 0;
}

int write_socket(char *buf, int len)
{
	if (socketFd == 0)
	{
		DLP_DBG("socket is NULL!\n");
		return -1;
	}
	if (send(socketFd, buf, len, 0) == -1)
	{
		DLP_DBG("socket send() FAIL!\n");
		return -1;
	}
	return 0;
}

int pakage_socket_cmd(char cmdId, int value, char **buf, int *len)
{
	int pakage_len = 0;
	int value_len = 0;
	char negative = 0;
	int i = 0;
    int j = 0;
	char val_str[15];
	
	if (value < 0)
	{
		negative = 1;
		value = -value;
	}

    sprintf(val_str, "%d", value);
	value_len = strlen(val_str);

    DLP_DBG("val_str %s, val_len %d!\n", val_str, value_len);
	
	pakage_len = SOCKET_HEAD_LEN + SOCKET_CMDID_LEN 
        + SOCKET_NEGATIVE_BYTE + 1 + value_len;    //1 is for negative byte
	*len = pakage_len;
	*buf = (char *)malloc(pakage_len);
	if (*buf == NULL)
	{
		return -1;
	}
	(*buf)[i++] = SOCKET_HEAD1;
	(*buf)[i++] = SOCKET_HEAD2;
	(*buf)[i++] = cmdId;
    (*buf)[i++] = value_len + 1;
	(*buf)[i++] = negative;
    for(j=0; j<value_len; j++)
    {
        (*buf)[i++] = val_str[j]-'0';
    }

    
    DLP_DBG("cmdId %d, pakage_len %d!\n", cmdId, pakage_len);
    for (j=0; j < pakage_len; j++)
    {
        DLP_DBG("pakage[%d] %d!\n", j, (*buf)[j]);
    }
	return 0;
}

int set_mounting_mode(EN_MountingMode mode)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_MOUNTINT_MODE, mode, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_vertical_keystone(int value)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_VERTICAL_KS, value, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_factory_reset()
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_FACTORY_RST, 1, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_backlight(int value)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_BACKLIGHT, value, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_key_event(EN_KeyEvent key)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_KEY_EVENT, key, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_open_service_menu()
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_DLP_MENU, 1, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_3d_mode(EN_3dMode mode)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_3D_MODE, mode, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_display_mode(EN_DisplayMode mode)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_DISPLAY_MODE, mode, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_red_pwm_duty(int value)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_SET_R_DUTY, value, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_green_pwm_duty(int value)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_SET_G_DUTY, value, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_blue_pwm_duty(int value)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_SET_B_DUTY, value, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int set_yellow_pwm_duty(int value)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_SET_Y_DUTY, value, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

int get_color_pwm_duty(int value)
{
	char *buf = NULL;
	int len = 0;
	if (pakage_socket_cmd(CMD_GET_DUTY, value, &buf, &len))
	{
		return -1;
	}
	
	write_socket(buf, len);
	free(buf);
	buf = NULL;
	
	return 0;
}

/*
int dlp_read_client_socket(int fd, int8 *buf, int len)
{
	int cmdlen = read(fd, buf, len);
	if(cmdlen <= 0){
		//****delete socket from socket list when socket disconnented*********
		DLP_EOR("socket client:%d disconnected!delete socketid list!\n", fd);
		dlp_delete_socket_by_fd(fd);
		return 0;
	}
	return cmdlen;

}
*/

int socket_read()
{
    unsigned int len;
    int i = 0;
    int turn = 0;
    int8 *data = (int8*)malloc(20);
    memset(data, 0, 20);
    
    while(1)
    {
        len = dlp_read_client_socket(socketFd, data, 20);
        DLP_DBG("receive data len=%d\n", len);
        
        for(i=0; i<len; i++)
        {
            DLP_DBG("socket rcv data %x\n", data[i]);
        }
        sleep(1);

        switch (turn)
        {
            case 0:
                set_mounting_mode(MM_DESKTOP_FRONT);
                turn++;
                break;
            case 1:
                set_vertical_keystone(0);
                turn++;
                break;

            case 2:
                set_factory_reset();
                turn++;
                break;

            case 3:
                set_backlight(100);
                turn++;
                break;

            case 4:
                set_key_event(KEY_UP);
                turn++;
                break;

            case 5:
                set_open_service_menu();
                turn++;
                break;

            case 6:
                set_3d_mode(E3D_LEFT_RIGHT);
                turn++;
                break;

            case 7:
                set_display_mode(DM_GORGEOUS);
                turn++;
                break;

            case 8:
                set_vertical_keystone(-10);
                turn++;
                break;

            default:
                turn = 0;
                break;
        }
    }

}

/*
void dlp_socket_test_create()
{
    socket_connect();
    socket_read();
}
*/

#if 0
void dlp_reply_to_socket(int8 *uartData, int uartLen)
{
    int8 *socketData = NULL;
    int dataLen = 0;
    int *socketFd = NULL;
    int fdCount = 0;
    int8 cmdId = CMDID_UNKNOW;
    int i = 0;
    cmdId = dlp_pckage_to_socket_data(uartData, uartLen, &socketData, &dataLen);
    if (cmdId == CMDID_UNKNOW)
    {
        DLP_EOR("receive unknow cmd\n");
        return ;
    }
    fdCount = dlp_find_socket_by_cmdid(&socketFd, cmdId);
    DLP_EOR("receive cmdid %x, found socket %d\n", cmdId, fdCount);//TEST
    for (i=0; i<dataLen; i++)
    {
        DLP_EOR("reply socketData[%d] = %x\n", i, socketData[i]);
    }
    i = 0;
	while(i < fdCount)
    {
        if (socketFd == NULL)
        {
            DLP_EOR("socketFd NULL\n");//TEST
        }
        DLP_EOR("START TO handle socketFd \n");//TEST
		if(socketFd[i] > 0)
        {
            DLP_EOR("handle fd %d\n", socketFd[i]);//TEST
			fd_set write_fd;
			FD_ZERO(&write_fd);
			FD_SET(socketFd[i], &write_fd);
					
			/* selecting the time out value */
			struct timeval to;
			to.tv_sec = 0;
			to.tv_usec = 0;
			
			/* select() takes (max of all fds being passed) + 1 as first arg,
			* we only have one fd so passing fd + 1 */
			int32_t len = select(socketFd[i]+1, NULL, &write_fd, NULL, &to);
			if (len < 0)
            {
				DLP_EOR("select failed:%s\n", strerror(errno));
				i++;
				continue;
			}
            DLP_EOR("select success!!\n");//TEST
			if (FD_ISSET(socketFd[i], &write_fd))
            {
                DLP_EOR("fd found and write back!\n");//TEST
				write(socketFd[i], socketData, dataLen);
			}
            else
            {
				DLP_EOR("socketid:%d is not writeable\n", socketFd[i]);
			}
		}
        else
        {
			DLP_EOR("error socketid:%d\n", socketFd[i]);
		}
		i++;
	}
    if (fdCount > 0)
    {
        free(socketFd);
    }
    free(socketData);
}
#endif












