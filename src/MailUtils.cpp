#include "../include/MailUtils.h"
#include <ESP_Mail_Client.h>

// --- Email Config ---
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "unclezero1234@gmail.com"
#define AUTHOR_PASSWORD "elju njgp svki wbll" 
#define RECIPIENT_EMAIL "harel.grecht@gmail.com"

SMTPSession smtp;

// Callback to track status
void smtpCallback(SMTP_Status status);

void mailSetUp() {
    smtp.debug(0); // Set to 1 for debug logs
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
    message.sender.name = "DodZero Gas Monitor";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = subject;
    message.addRecipient("Harel", RECIPIENT_EMAIL);
    message.text.content = messageText.c_str();

    Serial.println(">> Sending Email...");

    if (!smtp.connect(&session)) {
        Serial.println(">> Error connecting to SMTP");
        return;
    }

    if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println(">> Error sending Email: " + smtp.errorReason());
    }
}

void smtpCallback(SMTP_Status status) {
    if (status.success()) {
        Serial.println(">> Email sent successfully!");
    }
}