#ifndef MAIL_UTILS_H
#define MAIL_UTILS_H

#include <Arduino.h>

void mailSetUp();
void sendEmail(String subject, String messageText);


#endif