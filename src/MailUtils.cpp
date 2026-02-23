#include "../include/MailUtils.h"
#include <ESP_Mail_Client.h>

// --- Email Config ---
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

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
    session.login.email = authorEmail;
    session.login.password = authorPassword;
    session.login.user_domain = "";

    SMTP_Message message;
    message.sender.name = "DodZero Gas Monitor";
    message.sender.email = authorEmail;
    message.subject = subject;
    message.addRecipient("Harel", recipientEmail);
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