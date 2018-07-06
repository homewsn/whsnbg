/*
* Copyright (c) 2013-2018 Vladimir Alemasov
* All rights reserved
*
* This program and the accompanying materials are distributed under 
* the terms of GNU General Public License version 2 
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef MAINDLG_H_
#define MAINDLG_H_

#pragma once

#include <shellapi.h>		// for ShellExecute in atlctrlx.h
#include <atlctrlx.h>		// CWaitCursor, CMultiPaneStatusBarCtrl

#include "config.h"
#include "thread_udp.h"
#include "msg_mqtt_udp.h"
#include "msg_udp_mqtt.h"
#include "mqttsn.h"
#include "lists.h"
#include "sensor_data.h"

#ifdef _MSC_VER
#pragma warning (disable:4996)
#endif

#define IDT_TIMER_ACTIVE 1
#define IDT_TIMER_ASLEEP 2

static uint16_t MsgID = 1;

#define HOST_LEN		12
#define CLIENTID_LEN	24
#define TOPIC_LEN		50
#define MESSAGE_LEN		50
#define WILLTOPIC_LEN	50
#define WILLMESSAGE_LEN	50
#define REGTOPIC_LEN	50

// Использование WTL
// http://www.rsdn.ru/article/wtl/wtl-2.xml

class CMainDlg :	  public CDialogImpl<CMainDlg>
					, public CDialogResize<CMainDlg>
					, public CWinDataExchange<CMainDlg>
					, public CUpdateUI<CMainDlg>
					, public CMessageFilter
					, public CIdleHandler
{
protected:
	CIPAddressCtrl	m_ctrlHost;
	CComboBox		m_ctrlRegTopics;
	CMultiPaneStatusBarCtrl	m_ctrlStatusBar;
	CEdit			m_ctrlPackets;

protected:
	CString	m_HostStr;
	CString	m_ClientIdStr;
	DWORD	m_Host;
	UINT	m_Port;
	UINT	m_Duration;
	struct sockaddr_in m_addr;
	int		m_CleanSession;
	int		m_Dur0;
	int		m_Will;
	int		m_WillRetain;
	int		m_Lost;
	UINT	m_WillQos;
	CString	m_WillTopic;
	CString	m_WillMessage;
	int		m_Retain;
	UINT	m_Qos;
	CString	m_Topic;
	CString	m_Message;
	CString	m_RegTopic;
	uint16_t m_OnlineTopicID;
	uint16_t m_OnlineMsgID;
	list_clreg_t *m_RegTopics;
	mqttsn_client_state_t m_State;
	CString	m_Line;

public:
	enum { IDD = IDD_MAINDLG };

public:
	CMainDlg()
	{
	};

	virtual ~CMainDlg()
	{
	}

public:
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	//***** Overloaded CMessageLoop class functions

	//** PreTranslateMessage
	BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CWindow::IsDialogMessage(pMsg);
	}

	//** OnIdle (need AddIdleHandler in OnInitDialog and CIdleHandler interface)
	BOOL OnIdle()
	{
		UIEnable(IDC_STATICWILLTOPIC, m_Will);
		UIEnable(IDC_WILLTOPIC, m_Will);
		UIEnable(IDC_STATICWILLMESSAGE, m_Will);
		UIEnable(IDC_WILLMESSAGE, m_Will);
		UIEnable(IDC_WILLRETAIN, m_Will);
		UIEnable(IDC_STATICWILLQOS, m_Will);
		UIEnable(IDC_WILLQOS, m_Will);

		if (m_State == MQTTSN_CLIENT_DISCONNECTED ||
			m_State == MQTTSN_CLIENT_ASLEEP ||
			m_State == MQTTSN_CLIENT_AWAKE ||
			m_State == MQTTSN_CLIENT_LOST)
		{
			UIEnable(IDC_STATICPORT, 1);
			UIEnable(IDC_PORT, 1);
			UIEnable(IDC_STATICHOST, 1);
			UIEnable(IDC_HOST, 1);
			UIEnable(IDC_STATICCLIENTID, 1);
			UIEnable(IDC_CLIENTID, 1);
			UIEnable(IDC_STATICDURATION, 1);
			UIEnable(IDC_DURATION, 1);
			UIEnable(IDC_DUR0, 0);
			UIEnable(IDC_CLEANSESSION, 1);
			UIEnable(IDC_WILL, 1);

			UIEnable(IDC_STATICTOPIC, 0);
			UIEnable(IDC_TOPIC, 0);
			UIEnable(IDC_STATICREGTOPICS, 0);
			UIEnable(IDC_REGTOPICS, 0);
			UIEnable(IDC_STATICMESSAGE, 0);
			UIEnable(IDC_MESSAGE, 0);
			UIEnable(IDC_RETAIN, 0);
			UIEnable(IDC_STATICQOS, 0);
			UIEnable(IDC_QOS, 0);
			UIEnable(IDC_PUBLISH, 0);
			UIEnable(IDC_WILLTOPICUPDATE, 0);
			UIEnable(IDC_WILLMESSAGEUPDATE, 0);
			UIEnable(IDC_REGISTER, 0);
			UIEnable(IDC_SUB, 0);
			UIEnable(IDC_SUBREG, 0);
			UIEnable(IDC_UNSUB, 0);
			UIEnable(IDC_UNSUBREG, 0);

			if (m_State == MQTTSN_CLIENT_DISCONNECTED)
			{
				UIEnable(IDC_CONNECT, 1);
				UISetText(IDC_CONNECT, _T("Connect"));
			}
			if (m_State == MQTTSN_CLIENT_ASLEEP)
			{
				UIEnable(IDC_CONNECT, 1);
				UISetText(IDC_CONNECT, _T("Connect"));
			}
			if (m_State == MQTTSN_CLIENT_AWAKE)
			{
				UIEnable(IDC_CONNECT, 0);
				UISetText(IDC_CONNECT, _T("Auto"));
			}
			if (m_State == MQTTSN_CLIENT_LOST)
			{
				UIEnable(IDC_CONNECT, 1);
				UISetText(IDC_CONNECT, _T("Connect"));
			}
		}
		if (m_State == MQTTSN_CLIENT_ACTIVE)
		{
			UIEnable(IDC_STATICPORT, 0);
			UIEnable(IDC_PORT, 0);
			UIEnable(IDC_STATICHOST, 0);
			UIEnable(IDC_HOST, 0);
			UIEnable(IDC_STATICCLIENTID, 0);
			UIEnable(IDC_CLIENTID, 0);
			UIEnable(IDC_STATICDURATION, m_Dur0 == 0 ? 1 : 0);
			UIEnable(IDC_DURATION, m_Dur0 == 0 ? 1 : 0);
			UIEnable(IDC_DUR0, 1);
			UIEnable(IDC_CLEANSESSION, 0);
			UIEnable(IDC_WILL, 0);

			UIEnable(IDC_CONNECT, 1);
			UISetText(IDC_CONNECT, _T("Disconnect"));
			UIEnable(IDC_STATICTOPIC, 1);
			UIEnable(IDC_TOPIC, 1);
			UIEnable(IDC_WILLTOPICUPDATE, m_Will);
			UIEnable(IDC_WILLMESSAGEUPDATE, m_Will);
			UIEnable(IDC_REGISTER, 1);
			UIEnable(IDC_SUB, 1);
			UIEnable(IDC_UNSUB, 1);
			if (m_ctrlRegTopics.GetCount() == 0)
			{
				UIEnable(IDC_STATICREGTOPICS, 0);
				UIEnable(IDC_REGTOPICS, 0);
				UIEnable(IDC_STATICMESSAGE, 0);
				UIEnable(IDC_MESSAGE, 0);
				UIEnable(IDC_RETAIN, 0);
				UIEnable(IDC_STATICQOS, 0);
				UIEnable(IDC_QOS, 0);
				UIEnable(IDC_PUBLISH, 0);
				UIEnable(IDC_SUBREG, 0);
				UIEnable(IDC_UNSUBREG, 0);
			}
			else
			{
				UIEnable(IDC_STATICREGTOPICS, 1);
				UIEnable(IDC_REGTOPICS, 1);
				UIEnable(IDC_STATICMESSAGE, 1);
				UIEnable(IDC_MESSAGE, 1);
				UIEnable(IDC_RETAIN, 1);
				UIEnable(IDC_STATICQOS, 1);
				UIEnable(IDC_QOS, 1);
				UIEnable(IDC_PUBLISH, 1);
				UIEnable(IDC_SUBREG, 1);
				UIEnable(IDC_UNSUBREG, 1);
			}
		}
		UIUpdateChildWindows();

		DoDataExchange(FALSE, IDC_CLEANSESSION);
		DoDataExchange(FALSE, IDC_DUR0);
		DoDataExchange(FALSE, IDC_WILL);
		DoDataExchange(FALSE, IDC_WILLRETAIN);
		DoDataExchange(FALSE, IDC_RETAIN);

		return FALSE;
	}

	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	//***** Overloaded CWinDataExchange class functions

	//** OnDataValidateError
	void OnDataValidateError(UINT nCtrlID, BOOL bSave, _XData& data)
	{
		DataError(nCtrlID);
		CWinDataExchange<CMainDlg>::OnDataValidateError(nCtrlID, bSave, data);
	}

	//** OnDataExchangeError
	void OnDataExchangeError(UINT nCtrlID, BOOL bSave)
	{
		DataError(nCtrlID);
		CWinDataExchange<CMainDlg>::OnDataExchangeError(nCtrlID, bSave);
	}


private:
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	//***** private class functions

	//** initialization
	int Start(void)
	{
		m_Port = 1883;
		m_HostStr = _T("127.0.0.1");
		m_ClientIdStr = _T("ABCD");
		m_Duration = 120;
		m_Dur0 = 0;
		m_CleanSession = 0;
		m_Will = 1;
		m_WillRetain = 1;
		m_WillQos = 2;
		m_WillTopic = _T("sensors/ABCD");
		m_WillMessage = _T("offline");
		m_Retain = 1;
		m_Qos = 1;
		m_Topic = _T("sensors/#");
		m_Message = _T("online");
		m_Lost = 0;

		list_clreg_init(&m_RegTopics);

		msg_mqtt_udp_init();
		msg_udp_mqtt_init();

		SetClientState(MQTTSN_CLIENT_DISCONNECTED);

		if (thread_udp_start(m_hWnd) < 0)
			return -1;

		return 0;
	}

	//** exit
	void Stop(void)
	{
		thread_udp_stop();
	}

	//** convert gui to utf8 string
	void StrToUtf8(uint8_t **utf8str, uint16_t *len, CString str)
	{
#ifdef UNICODE
		int length = str.GetLength();
		*len = (uint16_t)WideCharToMultiByte(CP_UTF8, 0, str, length, 0, 0, 0, 0);
		*utf8str = (uint8_t *)malloc((size_t)*len);
		WideCharToMultiByte(CP_UTF8, 0, str, length, (LPSTR)*utf8str, (int)*len, 0, 0);
#else
		*len = (uint16_t)str.GetLength();
		*utf8str = (uint8_t *)malloc((size_t)*len);
		memcpy(*utf8str, (LPCSTR)str, (size_t)*len);
#endif
	}

	//** convert utf8 to gui string
	CString Utf8ToStr(uint8_t *utf8str, uint16_t len)
	{
#ifdef UNICODE
		CString str;
		int length = MultiByteToWideChar(CP_UTF8, 0, (LPSTR)utf8str, len, 0, 0);
		TCHAR *buf = str.GetBufferSetLength(length);
		MultiByteToWideChar(CP_UTF8, 0, (LPSTR)utf8str, len, buf, length);
		return str;
#else
		CString str = utf8str;
//		str->Format("%s", utf8str);
		return str;
#endif
	}

	//** 
	void mqttsn_command_decode(mqttsn_fixed_header_t *fixhdr)
	{
		unsigned char *buf;
		size_t size;

		if (fixhdr->msg_type == MQTTSN_WILLTOPICREQ)
		{
			mqttsn_willtopic_header_t willtopic = { 0 };

			m_Line.Format(_T("> WILLTOPICREQ\r\n"));
			AddNewLine();

			StrToUtf8(&willtopic.will_topic, &willtopic.will_topic_length, m_WillTopic);
			willtopic.flags.retain = m_WillRetain;
			willtopic.flags.qos = m_WillQos;
			mqttsn_willtopic_encode(&buf, &size, &willtopic);
			free(willtopic.will_topic);
			m_Line.Format(_T("< WILLTOPIC = WillTopic:%s, Retain:%d, QoS:%d\r\n"), m_WillTopic, m_WillRetain, m_WillQos);
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return;
		}
		if (fixhdr->msg_type == MQTTSN_WILLMSGREQ)
		{
			mqttsn_willmsg_header_t willmsg = { 0 };

			m_Line.Format(_T("> WILLMSGREQ\r\n"));
			AddNewLine();

			StrToUtf8(&willmsg.will_msg, &willmsg.will_msg_length, m_WillMessage);
			mqttsn_willmsg_encode(&buf, &size, &willmsg);
			free(willmsg.will_msg);
			m_Line.Format(_T("< WILLMSG = WillMsg:%s\r\n"), m_WillMessage);
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return;
		}
		if (fixhdr->msg_type == MQTTSN_CONNACK)
		{
			mqttsn_return_code_t code;
			mqttsn_connack_decode(fixhdr, &code);

			m_Line.Format(_T("> CONNACK = ReturnCode:%d\r\n"), code);
			AddNewLine();

			if (code == MQTTSN_ACCEPTED)
				SetClientState(MQTTSN_CLIENT_ACTIVE);

			return;
		}
		if (fixhdr->msg_type == MQTTSN_DISCONNECT)
		{
			uint16_t duration;
			mqttsn_disconnect_decode(fixhdr, &duration);

			m_Line.Format(_T("> DISCONNECT = Duration:%d\r\n"), duration);
			AddNewLine();

			if (m_Dur0 == 1)
				SetClientState(MQTTSN_CLIENT_DISCONNECTED);
			else
				SetClientState(MQTTSN_CLIENT_ASLEEP);

			return;
		}
		if (fixhdr->msg_type == MQTTSN_WILLTOPICRESP)
		{
			mqttsn_return_code_t code;
			mqttsn_willtopicresp_decode(fixhdr, &code);

			m_Line.Format(_T("> WILLTOPICRESP = ReturnCode:%d\r\n"), code);
			AddNewLine();
			return;
		}
		if (fixhdr->msg_type == MQTTSN_WILLMSGRESP)
		{
			mqttsn_return_code_t code;
			mqttsn_willmsgresp_decode(fixhdr, &code);

			m_Line.Format(_T("> WILLMSGRESP = ReturnCode:%d\r\n"), code);
			AddNewLine();
			return;
		}
		if (fixhdr->msg_type == MQTTSN_REGISTER)
		{
			mqttsn_register_header_t registerh = { 0 };
			mqttsn_regack_header_t regack = { 0 };
			list_clreg_t *ctop;

			mqttsn_register_decode(fixhdr, &registerh);
			ctop = list_clreg_add(&m_RegTopics, registerh.name, registerh.name_length, registerh.topic_id, registerh.msg_id);
			CString NameStr = Utf8ToStr(ctop->name, ctop->name_len);
			m_Line.Format(_T("> REGISTER = TopicName:%s, TopicID:%d, MsgID:%d\r\n"), NameStr, ctop->topic_id, ctop->msg_id);
			AddNewLine();
			if (m_ctrlRegTopics.FindStringExact(0, NameStr) == CB_ERR)
				m_ctrlRegTopics.SetCurSel(m_ctrlRegTopics.AddString(NameStr));

			regack.return_code = MQTTSN_ACCEPTED;
			regack.msg_id = registerh.msg_id;
			regack.topic_id = registerh.topic_id;
			mqttsn_regack_encode(&buf, &size, &regack);
			m_Line.Format(_T("< REGACK = ReturnCode:%d, TopicID:%d, MsgID:%d\r\n"), regack.return_code, regack.topic_id, regack.msg_id);
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return;
		}
		if (fixhdr->msg_type == MQTTSN_REGACK)
		{
			mqttsn_regack_header_t regack = { 0 };

			mqttsn_regack_decode(fixhdr, &regack);
			m_Line.Format(_T("> REGACK = ReturnCode:%d, TopicID:%d, MsgID:%d\r\n"), regack.return_code, regack.topic_id, regack.msg_id);
			AddNewLine();
			if (regack.return_code == MQTTSN_ACCEPTED)
			{
				list_clreg_t *ctop = list_clreg_find_msg_id(&m_RegTopics, regack.msg_id);
				if (ctop != NULL)
				{
					ctop->topic_id = regack.topic_id;
					CString NameStr = Utf8ToStr(ctop->name, ctop->name_len);
					if (m_ctrlRegTopics.FindStringExact(0, NameStr) == CB_ERR)
						m_ctrlRegTopics.SetCurSel(m_ctrlRegTopics.AddString(NameStr));
				}
			}
			return;
		}
		if (fixhdr->msg_type == MQTTSN_PUBLISH)
		{
			mqttsn_publish_header_t publish = { 0 };
			mqttsn_return_code_t code;
			mqttsn_puback_header_t puback;

			code = mqttsn_publish_decode(fixhdr, &publish);
			m_Line.Format(_T("> PUBLISH = Data:%s, TopicID:%d, MsgID:%d, Retain:%d, QoS:%d\r\n"), Utf8ToStr(publish.data, publish.data_length), publish.topic_id, publish.msg_id, publish.flags.retain, publish.flags.qos);
			AddNewLine();

			puback.topic_id = publish.topic_id;
			puback.msg_id = publish.msg_id;
			puback.return_code = code;
			if (publish.flags.qos > 0)
			{
				if (publish.flags.qos == 1)
				{
					mqttsn_puback_encode(&buf, &size, &puback);
					m_Line.Format(_T("< PUBACK = ReturnCode:%d, TopicID:%d, MsgID:%d\r\n"), puback.return_code, puback.topic_id, puback.msg_id);
				}
				else
				{
					mqttsn_pubrec_encode(&buf, &size, publish.msg_id);
					m_Line.Format(_T("< PUBREC = MsgID:%d\r\n"), publish.msg_id);
				}
				AddNewLine();
				msg_mqtt_udp_add_packet(&m_addr, buf, size);
			}

#ifdef SENSOR_DATA
			free(publish.data);
#endif

		}
		if (fixhdr->msg_type == MQTTSN_PUBREC)
		{
			uint16_t msg_id;

			mqttsn_pubrec_decode(fixhdr, &msg_id);
			m_Line.Format(_T("> PUBREC = MsgID:%d\r\n"), msg_id);
			AddNewLine();
			mqttsn_pubrel_encode(&buf, &size, msg_id);
			m_Line.Format(_T("< PUBREL = MsgID:%d\r\n"), msg_id);
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return;
		}
		if (fixhdr->msg_type == MQTTSN_PUBREL)
		{
			uint16_t msg_id;

			mqttsn_pubrel_decode(fixhdr, &msg_id);
			m_Line.Format(_T("> PUBREL = MsgID:%d\r\n"), msg_id);
			AddNewLine();
			mqttsn_pubcomp_encode(&buf, &size, msg_id);
			m_Line.Format(_T("< PUBCOMP = MsgID:%d\r\n"), msg_id);
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return;
		}
		if (fixhdr->msg_type == MQTTSN_PUBCOMP)
		{
			uint16_t msg_id;

			mqttsn_pubcomp_decode(fixhdr, &msg_id);
			m_Line.Format(_T("> PUBCOMP = MsgID:%d\r\n"), msg_id);
			AddNewLine();
			return;
		}
		if (fixhdr->msg_type == MQTTSN_PUBACK)
		{
			mqttsn_puback_header_t puback = { 0 };

			mqttsn_puback_decode(fixhdr, &puback);
			m_Line.Format(_T("> PUBACK = ReturnCode:%d, TopicID:%d, MsgID:%d\r\n"), puback.return_code, puback.topic_id, puback.msg_id);
			AddNewLine();
			return;
		}
		if (fixhdr->msg_type == MQTTSN_PINGREQ)
		{
			mqttsn_pingreq_header_t pingreq = { 0 };

			mqttsn_pingreq_decode(fixhdr, &pingreq);
			m_Line.Format(_T("> PINGREQ = ClientID: isn't supported yet\r\n"));
			AddNewLine();
			mqttsn_pingresp_encode(&buf, &size);
			m_Line.Format(_T("< PINGRESP\r\n"));
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return;
		}
		if (fixhdr->msg_type == MQTTSN_PINGRESP)
		{
			m_Line.Format(_T("> PINGRESP\r\n"));
			AddNewLine();
			if (m_State == MQTTSN_CLIENT_AWAKE)
				SetClientState(MQTTSN_CLIENT_ASLEEP);
			return;
		}
		if (fixhdr->msg_type == MQTTSN_SUBACK)
		{
			mqttsn_suback_header_t suback = { 0 };

			mqttsn_suback_decode(fixhdr, &suback);
			m_Line.Format(_T("> SUBACK = ReturnCode:%d, TopicID:%d, MsgID:%d, QoS:%d\r\n"), suback.return_code, suback.topic_id, suback.msg_id, suback.flags.qos);
			AddNewLine();
			return;
		}
		if (fixhdr->msg_type == MQTTSN_UNSUBACK)
		{
			uint16_t msg_id;

			mqttsn_unsuback_decode(fixhdr, &msg_id);
			m_Line.Format(_T("> UNSUBACK = MsgID:%d\r\n"), msg_id);
			AddNewLine();
			return;
		}
	}

	void DataError(UINT nCtrlID)
	{
		TCHAR buf[100];

		if (nCtrlID == IDC_PORT)
			_stprintf(buf, _T("Port must be [1000..65535]"));
		if (nCtrlID == IDC_CLIENTID)
			_stprintf(buf, _T("Client ID must be [1..%d] symbols length"), CLIENTID_LEN /*sizeof(m_ClientIdStr) * sizeof(TCHAR) */);
		if (nCtrlID == IDC_DURATION)
			_stprintf(buf, _T("Duration must be [5..65535]"));
		if (nCtrlID == IDC_WILLTOPIC)
			_stprintf(buf, _T("Will topic must be [1..%d] symbols length"), WILLTOPIC_LEN /*sizeof(m_WillTopic) * sizeof(TCHAR)*/);
		if (nCtrlID == IDC_WILLMESSAGE)
			_stprintf(buf, _T("Will message must be [1..%d] symbols length"), WILLMESSAGE_LEN /*sizeof(m_WillMessage) * sizeof(TCHAR)*/);
		if (nCtrlID == IDC_WILLQOS)
			_stprintf(buf, _T("Will QoS must be [0..2]"));
		if (nCtrlID == IDC_TOPIC)
			_stprintf(buf, _T("Topic must be [1..%d] symbols length"), TOPIC_LEN /*sizeof(m_Topic) * sizeof(TCHAR)*/);
		if (nCtrlID == IDC_MESSAGE)
			_stprintf(buf, _T("Message must be [1..%d] symbols length"), MESSAGE_LEN /*sizeof(m_Message) * sizeof(TCHAR)*/);
		if (nCtrlID == IDC_QOS)
			_stprintf(buf, _T("QoS must be [0..2]"));
		MessageBox(buf, _T("sensim"), MB_OK | MB_ICONERROR);
	}

	void AddNewLine()
	{
		static int line = 1;

		int len = m_ctrlPackets.GetWindowTextLength() * sizeof(TCHAR);
		TCHAR *buf = (TCHAR *)malloc(len + sizeof(TCHAR) + m_Line.GetLength() * sizeof(TCHAR));
		ATLASSERT(buf != NULL);
		if (len == 0)
			*buf = 0;
		else
			m_ctrlPackets.GetWindowText(buf, len + 1);
		_tcscat(buf, m_Line);
		m_ctrlPackets.SetWindowText(buf);
		m_ctrlPackets.LineScroll(line);
		++line;
		free(buf);
	}

	void SetClientState(mqttsn_client_state_t state)
	{
		m_State = state;

		KillTimer(IDT_TIMER_ACTIVE);
		KillTimer(IDT_TIMER_ASLEEP);
		if (m_State == MQTTSN_CLIENT_ACTIVE)
			SetTimer(IDT_TIMER_ACTIVE, (m_Duration * 1000 * 4) / 5);
		if (m_State == MQTTSN_CLIENT_ASLEEP)
			SetTimer(IDT_TIMER_ASLEEP, (m_Duration * 1000 * 4) / 5);

		if (m_State == MQTTSN_CLIENT_DISCONNECTED)
			UISetText(0, _T("Client status: Disconnected"));
		if (m_State == MQTTSN_CLIENT_ACTIVE)
			UISetText(0, _T("Client status: Active"));
		if (m_State == MQTTSN_CLIENT_ASLEEP)
			UISetText(0, _T("Client status: Asleep"));
		if (m_State == MQTTSN_CLIENT_AWAKE)
			UISetText(0, _T("Client status: Awake"));
		if (m_State == MQTTSN_CLIENT_LOST)
			UISetText(0, _T("Client status: Lost"));
		UIUpdateStatusBar();
	}



public:
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	//***** static class functions

	//** 


public:
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	//***** classes maps

	//** CDialogImplBaseT class map
	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_MQTTSN_MSG, OnMqttsMsg)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		COMMAND_ID_HANDLER(IDCANCEL, OnClose)
		COMMAND_ID_HANDLER(IDC_CONNECT, OnConnect)
		COMMAND_ID_HANDLER(IDC_CLEANSESSION, OnCleanSession)
		COMMAND_ID_HANDLER(IDC_DUR0, OnDur0)
		COMMAND_ID_HANDLER(IDC_WILL, OnWill)
		COMMAND_ID_HANDLER(IDC_WILLRETAIN, OnWillRetain)
		COMMAND_ID_HANDLER(IDC_RETAIN, OnRetain)
		COMMAND_ID_HANDLER(IDC_LOST, OnLost)
		COMMAND_ID_HANDLER(IDC_WILLTOPICUPDATE, OnWillTopicUpdate)
		COMMAND_ID_HANDLER(IDC_WILLMESSAGEUPDATE, OnWillMessageUpdate)
		COMMAND_ID_HANDLER(IDC_REGISTER, OnRegister)
		COMMAND_ID_HANDLER(IDC_PUBLISH, OnPublish)
		COMMAND_ID_HANDLER(IDC_SUB, OnSub)
		COMMAND_ID_HANDLER(IDC_SUBREG, OnSubReg)
		COMMAND_ID_HANDLER(IDC_UNSUB, OnUnSub)
		COMMAND_ID_HANDLER(IDC_UNSUBREG, OnUnSubReg)
		CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
		REFLECT_NOTIFICATIONS()
	END_MSG_MAP()

	//** CDialogResize class map
	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_CLIENTID, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_DURATION, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_DUR0, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CLEANSESSION, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_WILL, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_WILLTOPIC, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_WILLRETAIN, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_WILLMESSAGE, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_WILLQOS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_STATICWILLQOS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_CONNECT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_TOPIC, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_RETAIN, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REGTOPICS, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_MESSAGE, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_QOS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_STATICQOS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_PUBLISH, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_WILLTOPICUPDATE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_WILLMESSAGEUPDATE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_REGISTER, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_SUB, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_SUBREG, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_UNSUB, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_UNSUBREG, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_LOST, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_PACKETS, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(ATL_IDW_STATUS_BAR, DLSZ_SIZE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	//** CWinDataExchange class map
	BEGIN_DDX_MAP(CMainDlg)
		DDX_CONTROL_HANDLE(IDC_HOST, m_ctrlHost)
		DDX_TEXT_LEN(IDC_HOST, m_HostStr, HOST_LEN /*sizeof(m_HostStr)*/)
		DDX_UINT_RANGE(IDC_PORT, m_Port, (UINT)1000, (UINT)65535)
		DDX_TEXT_LEN(IDC_CLIENTID, m_ClientIdStr, CLIENTID_LEN /* sizeof(m_ClientIdStr) - 1 */)
		DDX_UINT_RANGE(IDC_DURATION, m_Duration, (UINT)5, (UINT)65535)
		DDX_CHECK(IDC_DUR0, m_Dur0)
		DDX_CHECK(IDC_CLEANSESSION, m_CleanSession)
		DDX_CHECK(IDC_WILL, m_Will)
		DDX_TEXT_LEN(IDC_WILLTOPIC, m_WillTopic, WILLTOPIC_LEN /* sizeof(m_WillTopic) - 1 */)
		DDX_TEXT_LEN(IDC_WILLMESSAGE, m_WillMessage, WILLMESSAGE_LEN /* sizeof(m_WillMessage) - 1 */)
		DDX_CHECK(IDC_WILLRETAIN, m_WillRetain)
		DDX_UINT_RANGE(IDC_WILLQOS, m_WillQos, (UINT)0, (UINT)2)
		DDX_TEXT_LEN(IDC_TOPIC, m_Topic, TOPIC_LEN /* sizeof(m_Topic) - 1 */)
		DDX_CONTROL_HANDLE(IDC_REGTOPICS, m_ctrlRegTopics)
		DDX_TEXT_LEN(IDC_MESSAGE, m_Message, MESSAGE_LEN /* sizeof(m_Message) - 1 */)
		DDX_CHECK(IDC_RETAIN, m_Retain)
		DDX_CHECK(IDC_LOST, m_Lost)
		DDX_UINT_RANGE(IDC_QOS, m_Qos, (UINT)0, (UINT)2)
	END_DDX_MAP()

	//** CUpdateUI class map
	BEGIN_UPDATE_UI_MAP(CMainDlg)
		UPDATE_ELEMENT(IDC_STATICPORT, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_PORT, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICHOST, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_HOST, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICCLIENTID, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_CLIENTID, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICDURATION, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_DURATION, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_DUR0, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_CLEANSESSION, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_WILL, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_CONNECT, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICWILLTOPIC, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_WILLTOPIC, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICWILLMESSAGE, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_WILLMESSAGE, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_WILLRETAIN, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICWILLQOS, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_WILLQOS, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_PUBLISH, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICTOPIC, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_TOPIC, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICMESSAGE, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_MESSAGE, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICREGTOPICS, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_REGTOPICS, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_RETAIN, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_LOST, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_STATICQOS, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_QOS, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_WILLTOPICUPDATE, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_WILLMESSAGEUPDATE, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_REGISTER, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_SUB, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_SUBREG, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_UNSUB, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(IDC_UNSUBREG, UPDUI_CHILDWINDOW)
		UPDATE_ELEMENT(0, UPDUI_STATUSBAR)
	END_UPDATE_UI_MAP()


	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	//***** WM_ messages handlers

	//** WM_INITDIALOG
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		// set icons
		HICON hIcon = (HICON)::LoadImage(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		HICON hIconSmall = (HICON)::LoadImage(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SetIcon(hIcon, TRUE);
		SetIcon(hIconSmall, FALSE);

		// status bar
		HWND m_hWndStatusBar = ::CreateStatusWindow(SBARS_SIZEGRIP | CCS_BOTTOM | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, _T("Ready"), m_hWnd, ATL_IDW_STATUS_BAR);
		m_ctrlStatusBar.SubclassWindow(m_hWndStatusBar);

		m_ctrlPackets = GetDlgItem(IDC_PACKETS);

		DlgResize_Init(false);

//		LoadSize();
		CenterWindow();

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);
		UIAddChildWindowContainer(m_hWnd);
		UIAddStatusBar(m_hWndStatusBar);

		if (Start() < 0)
			::PostQuitMessage(EXIT_FAILURE);

		DoDataExchange(FALSE);

		return TRUE;
	}

	//** WM_DESTROY
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
//		SaveSize();
		Stop();

		// unregister message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->RemoveMessageFilter(this);
		pLoop->RemoveIdleHandler(this);
		return 0;
	}

	//** WM_MQTTSN_MSG -> mqtts packet has been received
	LRESULT OnMqttsMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		mqttsn_fixed_header_t fixhdr;
		msg_udp_mqtt_t *ms;

		while ((ms = msg_udp_mqtt_get_first()) != NULL)
		{
			if (mqttsn_fixed_header_decode(&fixhdr, (unsigned char *)ms->msg_buf, ms->msg_cnt) == 0)
				mqttsn_command_decode(&fixhdr);
			msg_udp_mqtt_remove(ms);
		}
		return 0;
	}

	//** WM_TIMER -> timer message
	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		mqttsn_pingreq_header_t pingreq = { 0 };

		if (wParam == IDT_TIMER_ACTIVE)
		{
			if (m_State != MQTTSN_CLIENT_ACTIVE)
			{
				ATLASSERT(0);
				KillTimer(IDT_TIMER_ACTIVE);
				return 0;
			}
			mqttsn_pingreq_encode(&buf, &size, &pingreq);
			m_Line.Format(_T("< PINGREQ\r\n"));
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return 0;
		}
		if (wParam == IDT_TIMER_ASLEEP)
		{
			if (m_State != MQTTSN_CLIENT_ASLEEP)
			{
				ATLASSERT(0);
				KillTimer(IDT_TIMER_ASLEEP);
				return 0;
			}
			if (m_Lost == 1)
			{
				SetClientState(MQTTSN_CLIENT_LOST);
				return 0;
			}
			StrToUtf8(&pingreq.client_id, &pingreq.client_id_length, m_ClientIdStr);
			mqttsn_pingreq_encode(&buf, &size, &pingreq);
			free(pingreq.client_id);
			m_Line.Format(_T("< PINGREQ = ClientID:%s\r\n"), m_ClientIdStr);
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			SetClientState(MQTTSN_CLIENT_AWAKE);
			return 0;
		}
		return 0;
	}




	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////
	//***** command messages handlers

	//** IDCANCEL (ALT+F4)
	LRESULT OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DestroyWindow();
		::PostQuitMessage(wID);
		return 0;
	}


	//** IDC_CONNECT
	LRESULT OnConnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;

		if (m_State == MQTTSN_CLIENT_DISCONNECTED || m_State == MQTTSN_CLIENT_ASLEEP || m_State == MQTTSN_CLIENT_LOST)
		{
			mqttsn_connect_header_t connect = { 0 };

			if (DoDataExchange(TRUE, IDC_PORT) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_CLIENTID) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_DURATION) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_CLEANSESSION) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_WILL) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_WILLRETAIN) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_WILLQOS) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_WILLTOPIC) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_WILLMESSAGE) == FALSE) return 0;
			m_ctrlHost.GetAddress(&m_Host);

			m_addr.sin_family = AF_INET;
			m_addr.sin_port = htons(m_Port);
			m_addr.sin_addr.s_addr = htonl(m_Host);

			StrToUtf8(&connect.client_id, &connect.client_id_length, m_ClientIdStr);
			connect.flags.clean_session = m_CleanSession;
			connect.flags.will = m_Will;
			connect.duration = m_Duration;

			mqttsn_connect_encode(&buf, &size, &connect);
			free(connect.client_id);
			m_Line.Format(_T("< CONNECT = ClientId:%s, CleanSession:%d, Will:%d, Duration:%d\r\n"), m_ClientIdStr, m_CleanSession, m_Will, m_Duration);
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return 0;
		}
		if (m_State == MQTTSN_CLIENT_ACTIVE)
		{
			uint16_t duration;

			if (DoDataExchange(TRUE, IDC_DUR0) == FALSE) return 0;
			if (DoDataExchange(TRUE, IDC_DURATION) == FALSE) return 0;

			if (m_Dur0 == 1)
				duration = 0;
			else
				duration = m_Duration;

			mqttsn_disconnect_encode(&buf, &size, duration);
			m_Line.Format(_T("< DISCONNECT = Duration:%d\r\n"), duration);
			AddNewLine();
			msg_mqtt_udp_add_packet(&m_addr, buf, size);
			return 0;
		}
		return 0;
	}

	//** IDC_CLEANSESSION
	LRESULT OnCleanSession(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DoDataExchange(TRUE, IDC_CLEANSESSION);
		return 0;
	}

	//** IDC_DUR0
	LRESULT OnDur0(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DoDataExchange(TRUE, IDC_DUR0);
		return 0;
	}

	//** IDC_WILL
	LRESULT OnWill(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DoDataExchange(TRUE, IDC_WILL);
		return 0;
	}

	//** IDC_WILLRETAIN
	LRESULT OnWillRetain(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DoDataExchange(TRUE, IDC_WILLRETAIN);
		return 0;
	}

	//** IDC_RETAIN
	LRESULT OnRetain(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DoDataExchange(TRUE, IDC_RETAIN);
		return 0;
	}

	//** IDC_LOST
	LRESULT OnLost(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		DoDataExchange(TRUE, IDC_LOST);
		return 0;
	}

	//** IDC_WILLTOPICUPDATE
	LRESULT OnWillTopicUpdate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		mqttsn_willtopic_header_t willtopic = { 0 };

		if (DoDataExchange(TRUE, IDC_WILLRETAIN) == FALSE) return 0;
		if (DoDataExchange(TRUE, IDC_WILLQOS) == FALSE) return 0;
		if (DoDataExchange(TRUE, IDC_WILLTOPIC) == FALSE) return 0;

		StrToUtf8(&willtopic.will_topic, &willtopic.will_topic_length, m_WillTopic);
		willtopic.flags.retain = m_WillRetain;
		willtopic.flags.qos = m_WillQos;
		mqttsn_willtopicupd_encode(&buf, &size, &willtopic);
		free(willtopic.will_topic);
		m_Line.Format(_T("< WILLTOPICUPD = WillTopic:%s, Retain:%d, QoS:%d\r\n"), m_WillTopic, m_WillRetain, m_WillQos);
		AddNewLine();
		msg_mqtt_udp_add_packet(&m_addr, buf, size);
		return 0;
	}

	//** IDC_WILLMESSAGEUPDATE
	LRESULT OnWillMessageUpdate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		mqttsn_willmsg_header_t willmsg = { 0 };

		if (DoDataExchange(TRUE, IDC_WILLMESSAGE) == FALSE) return 0;

		StrToUtf8(&willmsg.will_msg, &willmsg.will_msg_length, m_WillMessage);
		mqttsn_willmsgupd_encode(&buf, &size, &willmsg);
		free(willmsg.will_msg);
		m_Line.Format(_T("< WILLMSGUPD = WillMsg:%s\r\n"), m_WillMessage);
		AddNewLine();
		msg_mqtt_udp_add_packet(&m_addr, buf, size);
		return 0;
	}

	//** IDC_REGISTER
	LRESULT OnRegister(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		mqttsn_register_header_t registerh = { 0 };

		if (DoDataExchange(TRUE, IDC_TOPIC) == FALSE) return 0;

		StrToUtf8(&registerh.name, &registerh.name_length, m_Topic);
		m_OnlineMsgID = registerh.msg_id = MsgID;

		list_clreg_add_msg_id(&m_RegTopics, registerh.name, registerh.name_length, MsgID);

		MsgID = MsgID == 0xFFFF ? 0x0001 : ++MsgID;
		mqttsn_register_encode(&buf, &size, &registerh);
		free(registerh.name);
		m_Line.Format(_T("< REGISTER = TopicName:%s, TopicID:%d, MsgID:%d\r\n"), m_Topic, registerh.topic_id, registerh.msg_id);
		AddNewLine();
		msg_mqtt_udp_add_packet(&m_addr, buf, size);
		return 0;
	}

	//** IDC_PUBLISH
	LRESULT OnPublish(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		mqttsn_publish_header_t publish = { 0 };
		list_clreg_t *ctop;
		int sel;
		uint16_t name_len;
		uint8_t *name;
		CString NameStr;

		if (DoDataExchange(TRUE, IDC_MESSAGE) == FALSE) return 0;
		if (DoDataExchange(TRUE, IDC_RETAIN) == FALSE) return 0;
		if (DoDataExchange(TRUE, IDC_QOS) == FALSE) return 0;

		sel = m_ctrlRegTopics.GetCurSel();
		name_len = m_ctrlRegTopics.GetLBText(sel, NameStr);
		StrToUtf8(&name, &name_len, NameStr);
		ctop = list_clreg_find_name(&m_RegTopics, name, name_len);
		free(name);
		ATLASSERT(ctop != NULL);

		publish.flags.qos = m_Qos;
		publish.flags.retain = m_Retain;
		publish.flags.topic_id_type = MQTTSN_PREDEF_TOPIC_ID;
		StrToUtf8(&publish.data, &publish.data_length, m_Message);
		if (m_Qos != 0)
		{
			publish.msg_id = MsgID;
			MsgID = MsgID == 0xFFFF ? 0x0001 : ++MsgID;
		}
		publish.topic_id = ctop->topic_id;

		mqttsn_publish_encode(&buf, &size, &publish);
		free(publish.data);
		m_Line.Format(_T("< PUBLISH = Data:%s, TopicID:%d, MsgID:%d, Retain:%d, QoS:%d\r\n"), m_Message, publish.topic_id, publish.msg_id, publish.flags.retain, publish.flags.qos);
		AddNewLine();
		msg_mqtt_udp_add_packet(&m_addr, buf, size);
		return 0;
	}

	//** IDC_SUB
	LRESULT OnSub(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		mqttsn_subscribe_header_t subscribe = { 0 };

		if (DoDataExchange(TRUE, IDC_TOPIC) == FALSE) return 0;
		if (DoDataExchange(TRUE, IDC_QOS) == FALSE) return 0;

		StrToUtf8(&subscribe.name, &subscribe.name_length, m_Topic);
		subscribe.flags.topic_id_type = MQTTSN_TOPIC_NAME;
		subscribe.flags.qos = m_Qos;
		m_OnlineMsgID = subscribe.msg_id = MsgID;

		list_clreg_add_msg_id(&m_RegTopics, subscribe.name, subscribe.name_length, MsgID);

		MsgID = MsgID == 0xFFFF ? 0x0001 : ++MsgID;
		mqttsn_subscribe_encode(&buf, &size, &subscribe);
		free(subscribe.name);
		m_Line.Format(_T("< SUBSCRIBE = TopicName:%s, TopicIDType:%d, MsgID:%d\r\n"), m_Topic, subscribe.flags.topic_id_type, subscribe.msg_id);
		AddNewLine();
		msg_mqtt_udp_add_packet(&m_addr, buf, size);
		return 0;
	}

	//** IDC_SUBREG
	LRESULT OnSubReg(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		list_clreg_t *ctop;
		mqttsn_subscribe_header_t subscribe = { 0 };
		int sel;
		uint16_t name_len;
		uint8_t *name;
		CString NameStr;

		if (DoDataExchange(TRUE, IDC_QOS) == FALSE) return 0;

		sel = m_ctrlRegTopics.GetCurSel();
		name_len = m_ctrlRegTopics.GetLBText(sel, NameStr);
		StrToUtf8(&name, &name_len, NameStr);
		ctop = list_clreg_find_name(&m_RegTopics, name, name_len);
		free(name);
		ATLASSERT(ctop != NULL);

		subscribe.topic_id = ctop->topic_id;
		subscribe.flags.topic_id_type = MQTTSN_PREDEF_TOPIC_ID;
		subscribe.flags.qos = m_Qos;
		m_OnlineMsgID = subscribe.msg_id = MsgID;

		MsgID = MsgID == 0xFFFF ? 0x0001 : ++MsgID;
		mqttsn_subscribe_encode(&buf, &size, &subscribe);
		m_Line.Format(_T("< SUBSCRIBE = TopicID:%d, TopicIDType:%d, MsgID:%d\r\n"), subscribe.topic_id, subscribe.flags.topic_id_type, subscribe.msg_id);
		AddNewLine();
		msg_mqtt_udp_add_packet(&m_addr, buf, size);
		return 0;
	}

	//** IDC_UNSUB
	LRESULT OnUnSub(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		mqttsn_unsubscribe_header_t unsubscribe = { 0 };

		if (DoDataExchange(TRUE, IDC_TOPIC) == FALSE) return 0;

		StrToUtf8(&unsubscribe.name, &unsubscribe.name_length, m_Topic);
		unsubscribe.flags.topic_id_type = MQTTSN_TOPIC_NAME;
		m_OnlineMsgID = unsubscribe.msg_id = MsgID;

		MsgID = MsgID == 0xFFFF ? 0x0001 : ++MsgID;
		mqttsn_unsubscribe_encode(&buf, &size, &unsubscribe);
		free(unsubscribe.name);
		m_Line.Format(_T("< UNSUBSCRIBE = TopicName:%s, TopicIDType:%d, MsgID:%d\r\n"), m_Topic, unsubscribe.flags.topic_id_type, unsubscribe.msg_id);
		AddNewLine();
		msg_mqtt_udp_add_packet(&m_addr, buf, size);
		return 0;
	}

	//** IDC_UNSUBREG
	LRESULT OnUnSubReg(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		unsigned char *buf;
		size_t size;
		list_clreg_t *ctop;
		mqttsn_unsubscribe_header_t unsubscribe = { 0 };
		int sel;
		uint16_t name_len;
		uint8_t *name;
		CString NameStr;

		sel = m_ctrlRegTopics.GetCurSel();
		name_len = m_ctrlRegTopics.GetLBText(sel, NameStr);
		StrToUtf8(&name, &name_len, NameStr);
		ctop = list_clreg_find_name(&m_RegTopics, name, name_len);
		free(name);
		ATLASSERT(ctop != NULL);

		unsubscribe.topic_id = ctop->topic_id;
		unsubscribe.flags.topic_id_type = MQTTSN_PREDEF_TOPIC_ID;
		m_OnlineMsgID = unsubscribe.msg_id = MsgID;

		MsgID = MsgID == 0xFFFF ? 0x0001 : ++MsgID;
		mqttsn_unsubscribe_encode(&buf, &size, &unsubscribe);
		m_Line.Format(_T("< UNSUBSCRIBE = TopicID:%d, TopicIDType:%d, MsgID:%d\r\n"), unsubscribe.topic_id, unsubscribe.flags.topic_id_type, unsubscribe.msg_id);
		AddNewLine();
		msg_mqtt_udp_add_packet(&m_addr, buf, size);
		return 0;
	}
};

#endif // MAINDLG_H_
