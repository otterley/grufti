/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * got.h - process server messages
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef GOT_H
#define GOT_H

void gotmsg();
void gotctcp();
void gotdcc();
void gotaction();
void gotpublic();
void gotprivate();
void gotoper();
void gotopernotice();
void gotnotice();
void gotpongfromme();
void gotpublicnotice();
void gotmode();
void gotjoin();
void gotpart();
void goterror();
void gotpong();
void got001();
void got002();
void got003();
void got004();
void got212();
void got219();
void got250();
void got251();
void got252();
void got253();
void got254();
void got255();
void got265();
void got266();
void got301();
void got302();
void got315();
void got324();
void got329();
void got331();
void got332();
void got333();
void got352();
void got353();
void got366();
void got367();
void got368();
void got372();
void got375();
void got376();
void got377();
void got401();
void got403();
void got405();
void got421();
void got432();
void got433();
void got441();
void got451();
void got465();
void got471();
void got473();
void got474();
void got475();
void got482();
void gotquit();
void gotnick();
void gotkick();
void gotinvite();
void gottopic();
void gotping();
void gotunknown();
void gotunknownctcp();

#endif /* GOT_H */
