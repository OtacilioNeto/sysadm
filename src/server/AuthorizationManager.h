// ===============================
//  PC-BSD REST/JSON API Server
// Available under the 3-clause BSD License
// Written by: Ken Moore <ken@pcbsd.org> July 2015
// =================================
#ifndef _PCBSD_REST_AUTHORIZATION_MANAGER_H
#define _PCBSD_REST_AUTHORIZATION_MANAGER_H

#include "globals-qt.h"

class AuthorizationManager : public QObject{
	Q_OBJECT
public:
	AuthorizationManager();
	~AuthorizationManager();

	// == Token Interaction functions ==
	void clearAuth(QString token); //clear an authorization token
	bool checkAuth(QString token); //see if the given token is valid
	bool hasFullAccess(QString token); //see if the token is associated with a full-access account
	QString userForToken(QString token); //get the username associated with this token

	//SSL Certificate register/revoke/list (should only run if the current token is valid)
	bool RegisterCertificate(QString token, QString pubkey, QString nickname, QString email); //if token is valid, register the given cert for future logins
	static bool RegisterCertificateInternal(QString user, QByteArray pubkey, QString nickname, QString email); //INTERNAL ONLY
	bool RevokeCertificate(QString token, QString key, QString user=""); //user will be the current user if not empty - cannot touch other user's certs without full perms on current session
	void ListCertificates(QString token, QJsonObject *out);
	void ListCertificateChecksums(QJsonObject *out);

	int checkAuthTimeoutSecs(QString token); //Return the number of seconds that a token is valid for

	// == Token Generation functions
	QString LoginUP(QHostAddress host, QString user, QString pass); //Login w/ username & password
	QString LoginService(QHostAddress host, QString service); //Login a particular automated service

	//Stage 1 SSL Login Check: Generation of random string for this user
	QString GenerateEncCheckString();  //generate random string (server is receiver w/ pub key)
	QString GenerateEncString_bridge(QString str); //encrypt random string (server is initiator w/ private key)

	//Stage 2 SSL Login Check: Verify that the returned/encrypted string can be decoded and matches the initial random string
	QString LoginUC(QHostAddress host, QString encstring); 
	
	//Message Encryption/decryption methods
	QString encryptString(QString msg, QByteArray key);
	QString decryptString(QString msg, QByteArray key);

        //Additional SSL Encryption functions
        QList<QByteArray> GenerateSSLKeyPair(); //Returns: [public key, private key]
	QByteArray pubkeyForMd5(QString md5_base64);
	
private:
	QHash<QString, QDateTime> HASH;
	QHash <QString, QDateTime> IPFAIL;

	QString generateNewToken(bool isOperator, QString name);
	QStringList getUserGroups(QString user);
	bool local_checkActive(QString user);

	//Failure count management
	bool BumpFailCount(QString host);
	void ClearHostFail(QString host);

	//token->hashID filter simplification
	QString hashID(QString token){
	  QStringList tmp = QStringList(HASH.keys()).filter(token+"::::");
	  if(tmp.isEmpty()){ return ""; }
	  else{ return tmp.first(); }
	}
	
	//SSL Decrypt function
	QString DecryptSSLString(QString encstring, QString pubkey);

	//PAM login/check files
	bool pam_checkPW(QString user, QString pass);
	void pam_logFailure(int ret);
	
signals:
	void BlockHost(QHostAddress); //block a host address temporarily
	
};

#endif
