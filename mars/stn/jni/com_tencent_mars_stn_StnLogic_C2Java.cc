// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.

/**
 * created on : 2012-10-19
 * author : yanguoyue
 */

#include <jni.h>
#include <vector>

#include "stn_logic_C2Java.h"
#include "mars/comm/autobuffer.h"
#include "mars/comm/xlogger/xlogger.h"
#include "mars/comm/jni/util/var_cache.h"
#include "mars/comm/jni/util/scope_jenv.h"
#include "mars/comm/jni/util/comm_function.h"
#include "mars/comm/jni/util/scoped_jstring.h"
#include "mars/comm/compiler_util.h"
#include "mars/comm/platform_comm.h"
#include "mars/stn/stn.h"
#include "mars/stn/task_profile.h"
#include "mars/boost/signals2.hpp"

DEFINE_FIND_CLASS(KC2Java, "com/tencent/mars/stn/StnLogic")
DEFINE_FIND_CLASS(KC2JavaStnCgiProfile,"com/tencent/mars/stn/StnLogic$CgiProfile");

namespace mars {
namespace stn {

extern boost::signals2::signal<void (ErrCmdType _err_type, int _err_code, const std::string& _ip, uint16_t _port)> SignalOnLongLinkNetworkError;
extern boost::signals2::signal<void (ErrCmdType _err_type, int _err_code, const std::string& _ip, const std::string& _host, uint16_t _port)> SignalOnShortLinkNetworkError;
    
DEFINE_FIND_STATIC_METHOD(KC2Java_onTaskEnd, KC2Java, "onTaskEnd", "(ILjava/lang/Object;IILcom/tencent/mars/stn/StnLogic$CgiProfile;)I")
int C2Java_OnTaskEnd(uint32_t _taskid, void* const _user_context, const std::string& _user_id, int _error_type, int _error_code, const ConnectProfile& _profile){

    xverbose_function();
    xdebug2(TSF"recieve task profile: %_, %_, %_", _profile.start_connect_time, _profile.start_send_packet_time, _profile.read_packet_finished_time);

	VarCache* cache_instance = VarCache::Singleton();

	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();
    jclass cgiProfileCls = cache_instance->GetClass(env, KC2JavaStnCgiProfile);
    jmethodID jobj_init = env->GetMethodID(cgiProfileCls, "<init>", "()V");
    jobject jobj_cgiItem = env->NewObject(cgiProfileCls, jobj_init);
    if (nullptr == jobj_cgiItem) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"), "C2Java_OnTaskEnd: create jobject failed.");
        return -1;
    }

    jfieldID fid_taskStartTime = env->GetFieldID(cgiProfileCls, "taskStartTime","J");
    jfieldID fid_startConnectTime = env->GetFieldID(cgiProfileCls, "startConnectTime","J");
    jfieldID fid_connectSuccessfulTime = env->GetFieldID(cgiProfileCls, "connectSuccessfulTime","J");
    jfieldID fid_startHandshakeTime = env->GetFieldID(cgiProfileCls, "startHandshakeTime","J");
    jfieldID fid_handshakeSuccessfulTime = env->GetFieldID(cgiProfileCls, "handshakeSuccessfulTime","J");
    jfieldID fid_startSendPacketTime = env->GetFieldID(cgiProfileCls, "startSendPacketTime","J");
    jfieldID fid_startReadPacketTime = env->GetFieldID(cgiProfileCls, "startReadPacketTime","J");
    jfieldID fid_readPacketFinishedTime = env->GetFieldID(cgiProfileCls, "readPacketFinishedTime","J");
    jfieldID fid_rtt = env->GetFieldID(cgiProfileCls, "rtt","J");
    jfieldID fid_channelType = env->GetFieldID(cgiProfileCls, "channelType","I");
    

    uint64_t tls_start_time = _profile.tls_handshake_successful_time == 0 ? 0 : _profile.start_tls_handshake_time;
    env->SetLongField(jobj_cgiItem, fid_taskStartTime, _profile.start_time);
    env->SetLongField(jobj_cgiItem, fid_startConnectTime, _profile.start_connect_time);
    env->SetLongField(jobj_cgiItem, fid_connectSuccessfulTime, _profile.connect_successful_time);
    env->SetLongField(jobj_cgiItem, fid_startHandshakeTime,tls_start_time);
    env->SetLongField(jobj_cgiItem, fid_handshakeSuccessfulTime, _profile.tls_handshake_successful_time);
    env->SetLongField(jobj_cgiItem, fid_startSendPacketTime, _profile.start_send_packet_time);
    env->SetLongField(jobj_cgiItem, fid_startReadPacketTime, _profile.start_read_packet_time);
    env->SetLongField(jobj_cgiItem, fid_readPacketFinishedTime, _profile.read_packet_finished_time);
    env->SetLongField(jobj_cgiItem, fid_rtt, _profile.rtt_by_socket);
    env->SetIntField(jobj_cgiItem, fid_channelType, _profile.channel_type);

	int ret = (int)JNU_CallStaticMethodByMethodInfo(env, KC2Java_onTaskEnd, (jint)_taskid, _user_context, (jint)_error_type, (jint)_error_code, jobj_cgiItem).i;

	return ret;
};

DEFINE_FIND_STATIC_METHOD(KC2Java_onPush, KC2Java, "onPush", "(Ljava/lang/String;II[B)V")
void C2Java_OnPush(const std::string& _channel_id, uint32_t _cmdid, uint32_t _taskid, const AutoBuffer& _body, const AutoBuffer& _extend){

    xverbose_function();

	VarCache* cache_instance = VarCache::Singleton();

	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();

	jbyteArray data_jba = NULL;

	if (_body.Length() > 0) {
		data_jba = JNU_Buffer2JbyteArray(env, _body);
	} else {
		xdebug2(TSF"the data.Lenght() < = 0");
	}

	JNU_CallStaticMethodByMethodInfo(env, KC2Java_onPush, ScopedJstring(env, _channel_id.c_str()).GetJstr(), (jint)_cmdid, (jint)_taskid, data_jba);

	if (data_jba != NULL) {
		JNU_FreeJbyteArray(env, data_jba);
	}

};

DEFINE_FIND_STATIC_METHOD(KC2Java_onNewDns, KC2Java, "onNewDns", "(Ljava/lang/String;)[Ljava/lang/String;")
std::vector<std::string>  C2Java_OnNewDns(const std::string& _host){
	xverbose_function();

	VarCache* cache_instance = VarCache::Singleton();
	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();
	std::vector<std::string> iplist;

	if (!_host.empty()) {
		jobjectArray ip_strs = (jobjectArray)JNU_CallStaticMethodByMethodInfo(env, KC2Java_onNewDns, ScopedJstring(env, _host.c_str()).GetJstr()).l;
		if (ip_strs != NULL) {
			jsize size = env->GetArrayLength(ip_strs);
			for (int i = 0; i < size; i++) {
				jstring ip = (jstring)env->GetObjectArrayElement(ip_strs, i);
				if (ip != NULL) {
					iplist.push_back(ScopedJstring(env, ip).GetChar());
				}
				JNU_FreeJstring(env, ip);
			}
			env->DeleteLocalRef(ip_strs);
		}
	}
	else {
		xerror2(TSF"host is empty");
	}

	return iplist;
};

DEFINE_FIND_STATIC_METHOD(KC2Java_req2Buf, KC2Java, "req2Buf", "(ILjava/lang/Object;Ljava/io/ByteArrayOutputStream;[IILjava/lang/String;)Z")
bool C2Java_Req2Buf(uint32_t _taskid,  void* const _user_context, const std::string& _user_id, AutoBuffer& _outbuffer,  AutoBuffer& _extend, int& _error_code, const int _channel_select, const std::string& _host){
    xverbose_function();

	VarCache* cache_instance = VarCache::Singleton();
	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();

	//obtain the class ByteArrayOutputStream
	jclass byte_array_outputstream_clz = cache_instance->GetClass(env, "java/io/ByteArrayOutputStream");

	//obtain the method id of construct method
	jmethodID construct_mid = cache_instance->GetMethodId(env, byte_array_outputstream_clz, "<init>", "()V");

	//construct  the object of ByteArrayOutputStream
	jobject byte_array_output_stream_obj = env->NewObject(byte_array_outputstream_clz, construct_mid);


	jintArray errcode_array = env->NewIntArray(2);

	jboolean ret = JNU_CallStaticMethodByMethodInfo(env, KC2Java_req2Buf, (jint)_taskid, _user_context, byte_array_output_stream_obj, errcode_array, _channel_select, ScopedJstring(env, _host.c_str()).GetJstr()).z;

	if (ret) {
		jbyteArray ret_byte_array = (jbyteArray)JNU_CallMethodByName(env, byte_array_output_stream_obj, "toByteArray", "()[B").l;
		if (ret_byte_array != NULL) {
			jsize alen = env->GetArrayLength(ret_byte_array);
			jbyte* ba = env->GetByteArrayElements(ret_byte_array, NULL);
			_outbuffer.Write(ba, alen);
			env->ReleaseByteArrayElements(ret_byte_array, ba, 0);
			env->DeleteLocalRef(ret_byte_array);
		} else {
			xdebug2(TSF"the retByteArray is null");
		}
	}
	env->DeleteLocalRef(byte_array_output_stream_obj);

	jint* errcode = env->GetIntArrayElements(errcode_array, NULL);
	_error_code = errcode[0];
	env->ReleaseIntArrayElements(errcode_array, errcode, 0);
	env->DeleteLocalRef(errcode_array);

	return ret;
};

DEFINE_FIND_STATIC_METHOD(KC2Java_buf2Resp, KC2Java, "buf2Resp", "(ILjava/lang/Object;[B[II)I")
int C2Java_Buf2Resp(uint32_t _taskid, void* const _user_context, const std::string& _user_id, const AutoBuffer& _inbuffer, const AutoBuffer& _extend, int& _error_code, const int _channel_select){
    xverbose_function();

	VarCache* cache_instance = VarCache::Singleton();
	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();

	jbyteArray resp_buf_jba = NULL;

	if (_inbuffer.Length() > 0) {
		resp_buf_jba =  JNU_Buffer2JbyteArray(env, _inbuffer);
	} else {
		xdebug2(TSF"the decodeBuffer.Lenght() <= 0");
	}

	jintArray errcode_array = env->NewIntArray(1);

	jint ret = JNU_CallStaticMethodByMethodInfo(env, KC2Java_buf2Resp, (jint)_taskid, _user_context, resp_buf_jba, errcode_array, _channel_select).i;

	if (resp_buf_jba != NULL) {
		env->DeleteLocalRef(resp_buf_jba);
	}

    jint* errcode = env->GetIntArrayElements(errcode_array, NULL);
    _error_code = errcode[0];
    env->ReleaseIntArrayElements(errcode_array, errcode, 0);
    env->DeleteLocalRef(errcode_array);

	return ret;
};

DEFINE_FIND_STATIC_METHOD(KC2Java_makesureAuthed, KC2Java, "makesureAuthed", "(Ljava/lang/String;)Z")
bool C2Java_MakesureAuthed(const std::string& _host, const std::string& _user_id){
    xverbose_function();

    VarCache* cache_instance = VarCache::Singleton();
	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();

	jboolean ret = JNU_CallStaticMethodByMethodInfo(env, KC2Java_makesureAuthed, ScopedJstring(env, _host.c_str()).GetJstr()).z;

	return ret;
};

DEFINE_FIND_STATIC_METHOD(KC2Java_getLongLinkIdentifyCheckBuffer, KC2Java, "getLongLinkIdentifyCheckBuffer", "(Ljava/lang/String;Ljava/io/ByteArrayOutputStream;Ljava/io/ByteArrayOutputStream;[I)I")
int C2Java_GetLonglinkIdentifyCheckBuffer(const std::string& _channel_id, AutoBuffer& _identify_buffer, AutoBuffer& _buffer_hash, int32_t& _cmdid){
    xverbose_function();
    
    VarCache* cache_instance = VarCache::Singleton();
	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();
    
	//obtain the class ByteArrayOutputStream
	jclass byte_array_output_stream_clz = cache_instance->GetClass(env, "java/io/ByteArrayOutputStream");
    
	//obtain the method id of construct method
	jmethodID construct_mid = cache_instance->GetMethodId(env, byte_array_output_stream_clz, "<init>", "()V");
    
	//construct  the object of ByteArrayOutputStream
	jobject byte_array_outputstream_obj = env->NewObject(byte_array_output_stream_clz, construct_mid);
	jobject byte_array_outputstream_hash = env->NewObject(byte_array_output_stream_clz, construct_mid);

    jintArray jcmdid_array = env->NewIntArray(2);
    
	jint ret = 0;
	ret = JNU_CallStaticMethodByMethodInfo(env, KC2Java_getLongLinkIdentifyCheckBuffer, ScopedJstring(env, _channel_id.c_str()).GetJstr(), byte_array_outputstream_obj, byte_array_outputstream_hash, jcmdid_array).i;
	if (ret == kCheckNext || ret == kCheckNever)
	{
		xwarn2(TSF"getLongLinkIdentifyCheckBuffer uin == 0, not ready");
		env->DeleteLocalRef(byte_array_outputstream_obj);
		env->DeleteLocalRef(byte_array_outputstream_hash);
        env->DeleteLocalRef(jcmdid_array);
		return ret;
	}
    
	jbyteArray ret_byte_array = NULL;
	ret_byte_array = (jbyteArray)JNU_CallMethodByName(env, byte_array_outputstream_obj, "toByteArray", "()[B").l;
    
	jbyteArray ret_byte_hash = NULL;
	ret_byte_hash = (jbyteArray)JNU_CallMethodByName(env, byte_array_outputstream_hash, "toByteArray", "()[B").l;
    
    
    jint* jcmdids = env->GetIntArrayElements(jcmdid_array, NULL);
    _cmdid = (int)jcmdids[0];
    env->ReleaseIntArrayElements(jcmdid_array, jcmdids, 0);
    env->DeleteLocalRef(jcmdid_array);


	if (ret_byte_hash != NULL) {
		jsize alen2 = env->GetArrayLength(ret_byte_hash);
		jbyte* ba2 = env->GetByteArrayElements(ret_byte_hash, NULL);
		_buffer_hash.Write(ba2, alen2);
		env->ReleaseByteArrayElements(ret_byte_hash, ba2, 0);
		env->DeleteLocalRef(ret_byte_hash);
	}
	if (ret_byte_array != NULL) {
		jsize alen = env->GetArrayLength(ret_byte_array);
		jbyte* ba = env->GetByteArrayElements(ret_byte_array, NULL);
		_identify_buffer.Write(ba, alen);
		env->ReleaseByteArrayElements(ret_byte_array, ba, 0);
		env->DeleteLocalRef(ret_byte_array);
        
	} else {
		xdebug2(TSF"the retByteArray is NULL");
	}
    
	//free the local reference
	env->DeleteLocalRef(byte_array_outputstream_obj);
	env->DeleteLocalRef(byte_array_outputstream_hash);

	return ret;
};

DEFINE_FIND_STATIC_METHOD(KC2Java_onLongLinkIdentifyResp, KC2Java, "onLongLinkIdentifyResp", "(Ljava/lang/String;[B[B)Z")
bool C2Java_OnLonglinkIdentifyResponse(const std::string& _channel_id, const AutoBuffer& _response_buffer, const AutoBuffer& _identify_buffer_hash){
    xverbose_function();

	VarCache* cache_instance = VarCache::Singleton();

	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();

	jbyteArray data_jba = NULL;

	if (_response_buffer.Length() > 0) {
		data_jba = JNU_Buffer2JbyteArray(env, _response_buffer);
	} else {
		xdebug2(TSF"the respbuffer.Lenght() < = 0");
	}

	jbyteArray hash_jba = NULL;
	if (_identify_buffer_hash.Length() > 0) {
		hash_jba = JNU_Buffer2JbyteArray(env, _identify_buffer_hash);
	} else {
		xdebug2(TSF"the hashCodeBuffer.Lenght() < = 0");
	}

	jboolean ret = JNU_CallStaticMethodByMethodInfo(env, KC2Java_onLongLinkIdentifyResp, ScopedJstring(env, _channel_id.c_str()).GetJstr(),data_jba, hash_jba).z;

	if (data_jba != NULL) {
		JNU_FreeJbyteArray(env, data_jba);
	}

	if (hash_jba != NULL) {
		JNU_FreeJbyteArray(env, hash_jba);
	}

    return ret != 0;
};


DEFINE_FIND_STATIC_METHOD(KC2Java_trafficData, KC2Java, "trafficData", "(II)V")
void C2Java_TrafficData(ssize_t _send, ssize_t _recv) {

	VarCache* cache_instance = VarCache::Singleton();
	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();

	JNU_CallStaticMethodByMethodInfo(env, KC2Java_trafficData, (jint)_send, (jint)_recv);

};

DEFINE_FIND_STATIC_METHOD(KC2Java_reportNetConnectInfo, KC2Java, "reportConnectStatus", "(II)V")
void C2Java_ReportConnectStatus(int _all_connstatus, int _longlink_connstatus){
    xverbose_function();

    VarCache* cache_instance = VarCache::Singleton();
    ScopeJEnv scope_jenv(cache_instance->GetJvm());
    JNIEnv *env = scope_jenv.GetEnv();
    JNU_CallStaticMethodByMethodInfo(env, KC2Java_reportNetConnectInfo, (jint)_all_connstatus, (jint)_longlink_connstatus);
    xdebug2(TSF"all_connstatus = %0, longlink_connstatus = %_", _all_connstatus, _longlink_connstatus);
};

//DEFINE_FIND_STATIC_METHOD(KC2Java_reportCrashStatistics, KC2Java, "reportCrashStatistics", "(Ljava/lang/String;Ljava/lang/String;)V")
void reportCrashStatistics(const char* _raw, const char* _type)
{
}

DEFINE_FIND_STATIC_METHOD(KC2Java_requestSync, KC2Java, "requestDoSync", "()V")
void C2Java_RequestSync(){
    xverbose_function();

    VarCache* cache_instance = VarCache::Singleton();
    ScopeJEnv scope_jenv(cache_instance->GetJvm());
    JNIEnv *env = scope_jenv.GetEnv();
    JNU_CallStaticMethodByMethodInfo(env, KC2Java_requestSync);

};

DEFINE_FIND_STATIC_METHOD(KC2Java_requestNetCheckShortLinkHosts, KC2Java, "requestNetCheckShortLinkHosts", "()[Ljava/lang/String;")
void C2Java_RequestNetCheckShortLinkHosts(std::vector<std::string>& _hostlist){
	xverbose_function();

	VarCache* cache_instance = VarCache::Singleton();
	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();

	jobjectArray jobj_arr = (jobjectArray)JNU_CallStaticMethodByMethodInfo(env, KC2Java_requestNetCheckShortLinkHosts).l;

	if (jobj_arr != NULL) {
		jsize size = env->GetArrayLength(jobj_arr);
		for (int i = 0; i < size; i++) {
			jstring host = (jstring)env->GetObjectArrayElement(jobj_arr, i);
			if (host != NULL) {
				_hostlist.push_back(ScopedJstring(env, host).GetChar());
			}
			JNU_FreeJstring(env, host);
		}

		env->DeleteLocalRef(jobj_arr);
	}
};

DEFINE_FIND_STATIC_METHOD(KC2Java_reportTaskProfile, KC2Java, "reportTaskProfile", "(Ljava/lang/String;)V")
void C2Java_ReportTaskProfile(const TaskProfile& _task_profile){
	xverbose_function();

	VarCache* cache_instance = VarCache::Singleton();
	ScopeJEnv scope_jenv(cache_instance->GetJvm());
	JNIEnv *env = scope_jenv.GetEnv();

	XMessage profile_json;
	profile_json << "{";
	profile_json << "\"taskId\":" << _task_profile.task.taskid;
	profile_json << ",\"cmdId\":" << _task_profile.task.cmdid;
	profile_json << ",\"cgi\":\"" << _task_profile.task.cgi << "\"";
	profile_json << ",\"startTaskTime\":" << _task_profile.start_task_time;
	profile_json << ",\"endTaskTime\":" << _task_profile.end_task_time;
	profile_json << ",\"dyntimeStatus\":" << _task_profile.current_dyntime_status;
	profile_json << ",\"errCode\":" << _task_profile.err_code;
	profile_json << ",\"errType\":" << _task_profile.err_type;
	profile_json << ",\"channelSelect\":" << _task_profile.link_type;
	profile_json << ",\"historyNetLinkers\":[";
	std::vector<TransferProfile>::const_iterator iter = _task_profile.history_transfer_profiles.begin();
	for (; iter != _task_profile.history_transfer_profiles.end(); ) {
		const ConnectProfile& connect_profile = iter->connect_profile;
		profile_json << "{";
		profile_json << "\"startTime\":" << connect_profile.start_time;
		profile_json << ",\"dnsTime\":" << connect_profile.dns_time;
		profile_json << ",\"dnsEndTime\":" << connect_profile.dns_endtime;
		profile_json << ",\"connTime\":" << connect_profile.conn_time;
		profile_json << ",\"connErrCode\":" << connect_profile.conn_errcode;
		profile_json << ",\"tryIPCount\":" << connect_profile.tryip_count;
		profile_json << ",\"ip\":\"" << connect_profile.ip << "\"";
		profile_json << ",\"port\":" << connect_profile.port;
		profile_json << ",\"host\":\"" << connect_profile.host << "\"";
		profile_json << ",\"ipType\":" << connect_profile.ip_type;
		profile_json << ",\"disconnTime\":" << connect_profile.disconn_time;
		profile_json << ",\"disconnErrType\":" << connect_profile.disconn_errtype;
		profile_json << ",\"disconnErrCode\":" << connect_profile.disconn_errcode;

		profile_json << "}";
		if (++iter != _task_profile.history_transfer_profiles.end()) {
			profile_json << ",";
		}
		else {
			break;
		}
	}
	profile_json << "]}";
	std::string report_task_str = profile_json.String();

	JNU_CallStaticMethodByMethodInfo(env, KC2Java_reportTaskProfile, ScopedJstring(env, report_task_str.c_str()).GetJstr());
};
}
}
