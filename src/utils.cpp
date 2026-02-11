#include "../include/utils.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h>

const char* ssid = "LironWifi";
const char* password = "123456789";

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "unclezero1234@gmail.com"
#define AUTHOR_PASSWORD "elju njgp svki wbll"
#define RECIPIENT_EMAIL "harel.grecht@gmail.com"

SMTPSession smtp;

bool messageSent = false;

void smtpCallback(SMTP_Status status);

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.setSleep(false);

    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);

    int tryCount = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        tryCount++;
        
        if (tryCount > 40) { // 20 sec
            Serial.println("\nConnection Failed! Restarting...");
            delay(3000);
            ESP.restart();
        }
    }
    
    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void setupEmail() {
    smtp.debug(1);
    smtp.callback(smtpCallback);
}

void sendEmail(String subject, String messageText) {
    ESP_Mail_Session session;
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";

    SMTP_Message message;
    message.sender.name = "ESP32 Gas Monitor";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = subject;
    message.addRecipient("Harel", RECIPIENT_EMAIL);
    message.text.content = messageText.c_str();

    if (!smtp.connect(&session))
        return;

    if (!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());
}

void smtpCallback(SMTP_Status status) {
    Serial.println(status.info());
    if (status.success()) {
        Serial.println("--- Email sent successfully! ---");
    }
}