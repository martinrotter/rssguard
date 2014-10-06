<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="cs_CZ">
<context>
    <name>Application</name>
    <message>
        <source>Application is already running.</source>
        <translation>Aplikace již běží.</translation>
    </message>
</context>
<context>
    <name>DatabaseFactory</name>
    <message>
        <source>MySQL server works as expected.</source>
        <translation>MySQL server pracuje v pořádku.</translation>
    </message>
    <message>
        <source>No MySQL server is running in the target destination.</source>
        <translation>V daném umístění neběží žádný MySQL server.</translation>
    </message>
    <message>
        <source>Access denied. Invalid username or password used.</source>
        <extracomment>Access to MySQL server was denied.</extracomment>
        <translation>Přístup zamítnut. Nesprávne jméno či heslo.</translation>
    </message>
    <message>
        <source>Unknown error.</source>
        <extracomment>Unknown MySQL error arised.</extracomment>
        <translation>Neznámá chyba.</translation>
    </message>
</context>
<context>
    <name>FeedMessageViewer</name>
    <message>
        <source>Toolbar for messages</source>
        <translation>Panel zpráv</translation>
    </message>
    <message>
        <source>Feed update started</source>
        <extracomment>Text display in status bar when feed update is started.</extracomment>
        <translation>Spuštěn update kanálů</translation>
    </message>
    <message>
        <source>Updated feed &apos;%1&apos;</source>
        <extracomment>Text display in status bar when particular feed is updated.</extracomment>
        <translation>Aktualizován kanál &apos;%1&apos;</translation>
    </message>
    <message>
        <source>Cannot defragment database</source>
        <translation>Databázi nelze nefragmentovat</translation>
    </message>
    <message>
        <source>Database cannot be defragmented because feed update is ongoing.</source>
        <translation>Databázi nelze defragmentovat, protože právě běží aktualizace kanálů.</translation>
    </message>
    <message>
        <source>Database defragmented</source>
        <translation>Databáze defragmentována</translation>
    </message>
    <message>
        <source>Database was successfully defragmented.</source>
        <translation>Databáze byla úspěšně defragmentována.</translation>
    </message>
    <message>
        <source>Database was not defragmented</source>
        <translation>Database nedefragmentována</translation>
    </message>
    <message>
        <source>Database was not defragmented. This database backend does not support it or it cannot be defragmented now.</source>
        <translation>Databáze nebyla defragmentována. Tento typ databáze defragmentaci neumožňuje nebo databáze nemůže být defragmentována nyní.</translation>
    </message>
    <message>
        <source>Toolbar for feeds</source>
        <translation>Panel kanálů</translation>
    </message>
</context>
<context>
    <name>FeedsImportExportModel</name>
    <message>
        <source> (category)</source>
        <translation> (kategorie)</translation>
    </message>
    <message>
        <source> (feed)</source>
        <translation> (kanál)</translation>
    </message>
</context>
<context>
    <name>FeedsModel</name>
    <message>
        <source>Title</source>
        <extracomment>Title text in the feed list header.</extracomment>
        <translation>Nadpis</translation>
    </message>
    <message>
        <source>Titles of feeds/categories.</source>
        <extracomment>Feed list header &quot;titles&quot; column tooltip.</extracomment>
        <translation>Názvy kanálů/kategorií.</translation>
    </message>
    <message>
        <source>Counts of unread/all meesages.</source>
        <extracomment>Feed list header &quot;counts&quot; column tooltip.</extracomment>
        <translation>Počty nepřečtených/všech zpráviček.</translation>
    </message>
    <message>
        <source>Root</source>
        <extracomment>Name of root item of feed list which can be seen in feed add/edit dialog.</extracomment>
        <translation>Kořen</translation>
    </message>
    <message>
        <source>Invalid tree data.</source>
        <translation>Chybná data stromu.</translation>
    </message>
    <message>
        <source>Import successfull, but some feeds/categories were not imported due to error.</source>
        <translation>Import byl úspěšný, ale některé kanály či kategorie nebyly importovány kvůli chybě.</translation>
    </message>
    <message>
        <source>Import was completely successfull.</source>
        <translation>Import byl zcela úspěšný.</translation>
    </message>
</context>
<context>
    <name>FeedsModelCategory</name>
    <message numerus="yes">
        <source>%n unread message(s).</source>
        <extracomment>Tooltip for &quot;unread&quot; column of feed list.</extracomment>
        <translation>
            <numerusform>%n nepřečtená zpráva.</numerusform>
            <numerusform>%n nepřečtené zprávy.</numerusform>
            <numerusform>%n nepřečtených zpráv.</numerusform>
        </translation>
    </message>
    <message>
        <source>%1 (category)%2%3</source>
        <extracomment>Tooltip for standard feed.</extracomment>
        <translation>%1 (kategorie)%2%3</translation>
    </message>
    <message>
        <source>
This category does not contain any nested items.</source>
        <translation>
Tato kategorie neobsahuje žádné položky.</translation>
    </message>
</context>
<context>
    <name>FeedsModelFeed</name>
    <message>
        <source>does not use auto-update</source>
        <extracomment>Describes feed auto-update status.</extracomment>
        <translation>nepoužívá auto-aktualizace</translation>
    </message>
    <message>
        <source>uses global settings</source>
        <extracomment>Describes feed auto-update status.</extracomment>
        <translation>používá globální nastavení</translation>
    </message>
    <message numerus="yes">
        <source>uses specific settings (%n minute(s) to next auto-update)</source>
        <extracomment>Describes feed auto-update status.</extracomment>
        <translation>
            <numerusform>používá specifické nastavení (%n minuta do další aktualizace)</numerusform>
            <numerusform>používá specifické nastavení (%n minuty do další aktualizace)</numerusform>
            <numerusform>používá specifické nastavení (%n minut do další aktualizace)</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>%n unread message(s).</source>
        <extracomment>Tooltip for &quot;unread&quot; column of feed list.</extracomment>
        <translation>
            <numerusform>%n nepřečtená zpráva.</numerusform>
            <numerusform>%n nepřečtené zprávy.</numerusform>
            <numerusform>%n nepřečtených zpráv.</numerusform>
        </translation>
    </message>
    <message>
        <source>%1 (%2)%3

Network status: %6
Encoding: %4
Auto-update status: %5</source>
        <extracomment>Tooltip for feed.</extracomment>
        <translation>%1 (%2)%3

Síťový status: %6
Kódování: %4
Automatický update: %5</translation>
    </message>
</context>
<context>
    <name>FeedsModelRecycleBin</name>
    <message>
        <source>Recycle bin</source>
        <translation>Koš</translation>
    </message>
    <message>
        <source>Recycle bin contains all deleted messages from all feeds.</source>
        <translation>Koš obsahuje všechny smazané zprávy ze všech kanálů.</translation>
    </message>
    <message>
        <source>Recycle bin
%1</source>
        <translation>Koš
%1</translation>
    </message>
    <message numerus="yes">
        <source>%n deleted message(s).</source>
        <translation>
            <numerusform>%n smazaná zpráva.</numerusform>
            <numerusform>%n smazané zprávy.</numerusform>
            <numerusform>%n smazaných zpráv.</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>FeedsToolBar</name>
    <message>
        <source>Toolbar spacer</source>
        <translation>Mezera</translation>
    </message>
</context>
<context>
    <name>FeedsView</name>
    <message>
        <source>Context menu for feeds</source>
        <translation>Kontextové menu pro přehled kanálů</translation>
    </message>
    <message>
        <source>Cannot add standard category</source>
        <translation>Nelze přidat standardní kategorii</translation>
    </message>
    <message>
        <source>You cannot add new standard category now because feed update is ongoing.</source>
        <translation>Nyní nelze přidat standardní kategorii, protože právě probíhá aktualizace kanálů.</translation>
    </message>
    <message>
        <source>Cannot add standard feed</source>
        <translation>Nelze přidat standardní kanál</translation>
    </message>
    <message>
        <source>You cannot add new standard feed now because feed update is ongoing.</source>
        <translation>Nyní nelze přidat standardní kanál, protože právě probíhá aktualizace kanálů.</translation>
    </message>
    <message>
        <source>Cannot edit item</source>
        <translation>Nelze upravit položku</translation>
    </message>
    <message>
        <source>Selected item cannot be edited because feed update is ongoing.</source>
        <translation>Vybraná položka nemůže být nyní upravena, protože právě probíhá aktualizace kanálů.</translation>
    </message>
    <message>
        <source>Cannot delete item</source>
        <translation>Nelze smazat položku</translation>
    </message>
    <message>
        <source>Selected item cannot be deleted because feed update is ongoing.</source>
        <translation>Vybraná položka nemůže být nyní smazána, protože právě probíhá aktualizace kanálů.</translation>
    </message>
    <message>
        <source>Cannot update all items</source>
        <translation>Nelze aktualizovat všechny položky</translation>
    </message>
    <message>
        <source>You cannot update all items because another feed update is ongoing.</source>
        <translation>Právě nyní nemůžete aktualizovat všechny položky, protože nejspíše probíhá jiná aktualizace.</translation>
    </message>
    <message>
        <source>Cannot update selected items</source>
        <translation>Nelze aktualizovat vybrané položky</translation>
    </message>
    <message>
        <source>You cannot update selected items because another feed update is ongoing.</source>
        <translation>Právě nyní nemůžete aktualizovat vybrané položky, protože nejspíše probíhá jiná aktualizace.</translation>
    </message>
    <message>
        <source>You are about to delete selected feed or category.</source>
        <translation>Právě se chystáte smazat vybraný kanál či kategorii.</translation>
    </message>
    <message>
        <source>Deletion of item failed.</source>
        <translation>Mazání položky selhalo.</translation>
    </message>
    <message>
        <source>Selected item was not deleted due to error.</source>
        <translation>Vybraná položka nebyla smazána kvůli chybě.</translation>
    </message>
    <message>
        <source>Deleting feed or category</source>
        <translation>Mazání zprávy či kategorie</translation>
    </message>
    <message>
        <source>Do you really want to delete selected item?</source>
        <translation>Opravdu chcete vybranou položku smazat?</translation>
    </message>
    <message>
        <source>Permanently delete messages</source>
        <translation>Trvalé smazání zpráv</translation>
    </message>
    <message>
        <source>You are about to permanenty delete all messages from your recycle bin.</source>
        <translation>Chystáte se vysypat koš.</translation>
    </message>
    <message>
        <source>Do you really want to empty your recycle bin?</source>
        <translation>Opravdu chcete koš vysypat?</translation>
    </message>
    <message>
        <source>Context menu for empty space</source>
        <translation>Kontextové menu pro prázdný prostor</translation>
    </message>
    <message>
        <source>Context menu for recycle bin</source>
        <translation>Kontextové menu pro koš</translation>
    </message>
</context>
<context>
    <name>FormAbout</name>
    <message>
        <source>Information</source>
        <translation>Informace</translation>
    </message>
    <message>
        <source>Licenses</source>
        <translation>Licence</translation>
    </message>
    <message>
        <source>GNU GPL License (applies to RSS Guard source code)</source>
        <translation>GNU GPL Licence (pro kód programu RSS Guard)</translation>
    </message>
    <message>
        <source>GNU GPL License</source>
        <translation>GNU GPL Licence</translation>
    </message>
    <message>
        <source>BSD License (applies to QtSingleApplication source code)</source>
        <translation>BSD Licence (pro komponentu QtSingleApplication)</translation>
    </message>
    <message>
        <source>Licenses page is available only in English language.</source>
        <translation>Stránka Licence je dostupná pouze v anglickém jazyce.</translation>
    </message>
    <message>
        <source>Changelog</source>
        <translation>Historie verzí</translation>
    </message>
    <message>
        <source>Changelog page is available only in English language.</source>
        <translation>Historie verzí je dostupná pouze v anglickém jazyce.</translation>
    </message>
    <message>
        <source>License not found.</source>
        <translation>Licence nenalezena.</translation>
    </message>
    <message>
        <source>Changelog not found.</source>
        <translation>Historie změn nenalezena.</translation>
    </message>
    <message>
        <source>&lt;b&gt;%8&lt;/b&gt;&lt;br&gt;&lt;b&gt;Version:&lt;/b&gt; %1 (build on %2 with CMake %3)&lt;br&gt;&lt;b&gt;Revision:&lt;/b&gt; %4&lt;br&gt;&lt;b&gt;Build date:&lt;/b&gt; %5&lt;br&gt;&lt;b&gt;Qt:&lt;/b&gt; %6 (compiled against %7)&lt;br&gt;</source>
        <translation>&lt;b&gt;%8&lt;/b&gt;&lt;br&gt;&lt;b&gt;Verze:&lt;/b&gt; %1 (při sestavování použit OS %2 a CMake %3)&lt;br&gt;&lt;b&gt;Revize:&lt;/b&gt; %4&lt;br&gt;&lt;b&gt;Datum sestavení:&lt;/b&gt; %5&lt;br&gt;&lt;b&gt;Qt:&lt;/b&gt; %6 (při kompilaci %7)&lt;br&gt;</translation>
    </message>
    <message>
        <source>&lt;body&gt;%5 is a (very) tiny feed reader.&lt;br&gt;&lt;br&gt;This software is distributed under the terms of GNU General Public License, version 3.&lt;br&gt;&lt;br&gt;Contacts:&lt;ul&gt;&lt;li&gt;&lt;a href=&quot;mailto://%1&quot;&gt;%1&lt;/a&gt; ~email&lt;/li&gt;&lt;li&gt;&lt;a href=&quot;%2&quot;&gt;%2&lt;/a&gt; ~website&lt;/li&gt;&lt;/ul&gt;You can obtain source code for %5 from its website.&lt;br&gt;&lt;br&gt;&lt;br&gt;Copyright (C) 2011-%3 %4&lt;/body&gt;</source>
        <translation>&lt;body&gt;%5 je (velmi) jednoduduchá čtečka kanálů.&lt;br&gt;&lt;br&gt;Tato aplikace je šířena pod podmínkami licence GNU General Public License, verze 3.&lt;br&gt;&lt;br&gt;Kontakty:&lt;ul&gt;&lt;li&gt;&lt;a href=&quot;mailto://%1&quot;&gt;%1&lt;/a&gt; ~email&lt;/li&gt;&lt;li&gt;&lt;a href=&quot;%2&quot;&gt;%2&lt;/a&gt; ~web&lt;/li&gt;&lt;/ul&gt;Zdrojové kódy pro %5 je možné získat z jeho webu.&lt;br&gt;&lt;br&gt;&lt;br&gt;Copyright (C) 2011-%3 %4&lt;/body&gt;</translation>
    </message>
    <message>
        <source>About %1</source>
        <extracomment>About RSS Guard dialog title.</extracomment>
        <translation>O aplikaci %1</translation>
    </message>
    <message>
        <source>Paths</source>
        <translation>Cesty</translation>
    </message>
    <message>
        <source>Settings type</source>
        <translation>Typ nastavení</translation>
    </message>
    <message>
        <source>Settings file</source>
        <translation>Soubor nastavení</translation>
    </message>
    <message>
        <source>Database root path</source>
        <translation>Kořenový adresář databáze</translation>
    </message>
    <message>
        <source>FULLY portable</source>
        <translation>ZCELA portable</translation>
    </message>
    <message>
        <source>PARTIALLY portable</source>
        <translation>ČÁSTEČNĚ portable</translation>
    </message>
</context>
<context>
    <name>FormCategoryDetails</name>
    <message>
        <source>Parent category</source>
        <translation>Nadřazená kategorie</translation>
    </message>
    <message>
        <source>Select parent item for your category.</source>
        <translation>Zvolte nadřazenou kategorii pro Vaši kategorii.</translation>
    </message>
    <message>
        <source>Title</source>
        <translation>Nadpis</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Popis</translation>
    </message>
    <message>
        <source>Icon</source>
        <translation>Ikona</translation>
    </message>
    <message>
        <source>Select icon for your category.</source>
        <translation>Zvolte ikonu pro Vaši kategorii.</translation>
    </message>
    <message>
        <source>Add new category</source>
        <translation>Přidat novou kategorii</translation>
    </message>
    <message>
        <source>Edit existing category</source>
        <translation>Upravit existující kategorii</translation>
    </message>
    <message>
        <source>Cannot add category</source>
        <translation>Nelze přidat kategorii</translation>
    </message>
    <message>
        <source>Category was not added due to error.</source>
        <translation>Kategorie nebyla přidána kvůli chybě.</translation>
    </message>
    <message>
        <source>Cannot edit category</source>
        <translation>Nelze upravit kategorii</translation>
    </message>
    <message>
        <source>Category was not edited due to error.</source>
        <translation>Kategorie nebyla upravena kvůli chybě.</translation>
    </message>
    <message>
        <source>Category name is ok.</source>
        <translation>Název kategorie je v pořádku.</translation>
    </message>
    <message>
        <source>Category name is too short.</source>
        <translation>Název kategorie je příliš krátký.</translation>
    </message>
    <message>
        <source>Description is empty.</source>
        <translation>Popis je prázdný.</translation>
    </message>
    <message>
        <source>Select icon file for the category</source>
        <translation>Zvolte ikonu pro Vaši kategorii</translation>
    </message>
    <message>
        <source>Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)</source>
        <translation>Obrázky (*.bmp *.jpg *.jpeg *.png *.svg *.tga)</translation>
    </message>
    <message>
        <source>Select icon</source>
        <translation>Vybrat ikonu</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Zrušit</translation>
    </message>
    <message>
        <source>Look in:</source>
        <extracomment>Label to describe the folder for icon file selection dialog.</extracomment>
        <translation>Hledat v:</translation>
    </message>
    <message>
        <source>Icon name:</source>
        <translation>Název ikony:</translation>
    </message>
    <message>
        <source>Icon type:</source>
        <translation>Typ ikony:</translation>
    </message>
    <message>
        <source>Category title</source>
        <translation>Název kategorie</translation>
    </message>
    <message>
        <source>Set title for your category.</source>
        <translation>Zvolte název pro Vaši kategorii.</translation>
    </message>
    <message>
        <source>Category description</source>
        <translation>Popis kategorie</translation>
    </message>
    <message>
        <source>Set description for your category.</source>
        <translation>Zvolte popis Vaší kategorie.</translation>
    </message>
    <message>
        <source>Icon selection</source>
        <translation>Vybrat ikonu</translation>
    </message>
    <message>
        <source>Load icon from file...</source>
        <translation>Načíst ikonu ze souboru...</translation>
    </message>
    <message>
        <source>Do not use icon</source>
        <translation>Nepoužít ikonu</translation>
    </message>
    <message>
        <source>Use default icon</source>
        <translation>Použít výchozí ikonu</translation>
    </message>
    <message>
        <source>The description is ok.</source>
        <translation>Popis je v pořádku.</translation>
    </message>
</context>
<context>
    <name>FormFeedDetails</name>
    <message>
        <source>Parent category</source>
        <translation>Nadřazená kategorie</translation>
    </message>
    <message>
        <source>Select parent item for your feed.</source>
        <translation>Zvolte nadřazenou kategorii pro Váš kanál.</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>Typ</translation>
    </message>
    <message>
        <source>Select type of the standard feed.</source>
        <translation>Zvolte typ standardního kanálu.</translation>
    </message>
    <message>
        <source>Encoding</source>
        <translation>Kódování</translation>
    </message>
    <message>
        <source>Select encoding of the standard feed. If you are unsure about the encoding, then select &quot;UTF-8&quot; encoding.</source>
        <translation>Zvolte kódování kanálu. Pokud si nejste jisti, tak zvolte kódování &quot;UTF-8&quot;.</translation>
    </message>
    <message>
        <source>Auto-update</source>
        <translation>Auto-aktualizace</translation>
    </message>
    <message>
        <source>Select the auto-update strategy for this feed. Default auto-update strategy means that the feed will be update in time intervals set in application settings.</source>
        <translation>Zvolte strategii auto-aktualizací tohoto kanálu. Výchozí strategorie auto-aktualizace znamená, že kanál bude aktualizován v intervalech udaných v nastavení aplikace.</translation>
    </message>
    <message>
        <source> minutes</source>
        <translation> minut</translation>
    </message>
    <message>
        <source>Title</source>
        <translation>Nadpis</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Popis</translation>
    </message>
    <message>
        <source>URL</source>
        <translation></translation>
    </message>
    <message>
        <source>Fetch it now</source>
        <translation>Načíst nyní</translation>
    </message>
    <message>
        <source>Icon</source>
        <translation>Ikona</translation>
    </message>
    <message>
        <source>Select icon for your feed.</source>
        <translation>Zvolte ikonu pro Váš kanál.</translation>
    </message>
    <message>
        <source>Some feeds require authentication, including GMail feeds. BASIC, NTLM-2 and DIGEST-MD5 authentication schemes are supported.</source>
        <translation>Některé kanály vyžaduje autentizaci, a to včetně kanálů pro GMail. Je podporována autentizace BASIC, NTLM-2 a DIGEST-MD5.</translation>
    </message>
    <message>
        <source>Requires authentication</source>
        <translation>Vyžaduje autentizaci</translation>
    </message>
    <message>
        <source>Username</source>
        <translation>Uživatelské jméno</translation>
    </message>
    <message>
        <source>Password</source>
        <translation>Heslo</translation>
    </message>
    <message>
        <source>Fetch metadata</source>
        <translation>Načíst metadata</translation>
    </message>
    <message>
        <source>Add new feed</source>
        <translation>Přidat nový kanál</translation>
    </message>
    <message>
        <source>Edit existing feed</source>
        <translation>Upravit existující kanál</translation>
    </message>
    <message>
        <source>Feed name is ok.</source>
        <translation>Název kanálu je v pořádku.</translation>
    </message>
    <message>
        <source>Feed name is too short.</source>
        <translation>Název kanálu je příliš krátký.</translation>
    </message>
    <message>
        <source>Description is empty.</source>
        <translation>Popis je prázdný.</translation>
    </message>
    <message>
        <source>The url is ok.</source>
        <translation>Url je v pořádku.</translation>
    </message>
    <message>
        <source>The url does not meet standard pattern. Does your url start with &quot;http://&quot; or &quot;https://&quot; prefix.</source>
        <translation>Url neobsahuje standardní schéma. Začíná Vaše url schématem &quot;http://&quot; nebo &quot;https://&quot;.</translation>
    </message>
    <message>
        <source>The url is empty.</source>
        <translation>Url je prázdné.</translation>
    </message>
    <message>
        <source>Username is ok or it is not needed.</source>
        <translation>Uživatelské jméno je v pořádku nebo není třeba.</translation>
    </message>
    <message>
        <source>Username is empty.</source>
        <translation>Uživatelské jméno je prázdné.</translation>
    </message>
    <message>
        <source>Password is ok or it is not needed.</source>
        <translation>Heslo je v pořádku nebo není třeba.</translation>
    </message>
    <message>
        <source>Password is empty.</source>
        <translation>Heslo je prázdné.</translation>
    </message>
    <message>
        <source>Select icon file for the feed</source>
        <translation>Vybrat ikonu pro kanál</translation>
    </message>
    <message>
        <source>Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)</source>
        <translation>Obrázky (*.bmp *.jpg *.jpeg *.png *.svg *.tga)</translation>
    </message>
    <message>
        <source>Select icon</source>
        <translation>Vybrat ikonu</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Zrušit</translation>
    </message>
    <message>
        <source>Look in:</source>
        <extracomment>Label for field with icon file name textbox for selection dialog.</extracomment>
        <translation>Hledat v:</translation>
    </message>
    <message>
        <source>Icon name:</source>
        <translation>Název ikony:</translation>
    </message>
    <message>
        <source>Icon type:</source>
        <translation>Typ ikony:</translation>
    </message>
    <message>
        <source>Cannot add feed</source>
        <translation>Nelze přidat kanál</translation>
    </message>
    <message>
        <source>Feed was not added due to error.</source>
        <translation>Kanál nepřidán kvůli chybě.</translation>
    </message>
    <message>
        <source>Cannot edit feed</source>
        <translation>Nelze upravit kanál</translation>
    </message>
    <message>
        <source>All metadata fetched successfully.</source>
        <translation>Metadata stažena úspěšně.</translation>
    </message>
    <message>
        <source>Feed and icon metadata fetched.</source>
        <translation>Metadata a ikona staženy.</translation>
    </message>
    <message>
        <source>Result: %1.</source>
        <translation>Výsledek: %1.</translation>
    </message>
    <message>
        <source>Feed or icon metatada not fetched.</source>
        <translation>Metadata nebo ikona nestaženy.</translation>
    </message>
    <message>
        <source>Error: %1.</source>
        <translation>Chyba: %1.</translation>
    </message>
    <message>
        <source>No metadata fetched.</source>
        <translation>Žádná metadata nestažena.</translation>
    </message>
    <message>
        <source>Feed title</source>
        <translation>Název kanálu</translation>
    </message>
    <message>
        <source>Set title for your feed.</source>
        <translation>Zvolte název pro Váš kanál.</translation>
    </message>
    <message>
        <source>Feed description</source>
        <translation>Popis kanálu</translation>
    </message>
    <message>
        <source>Set description for your feed.</source>
        <translation>Zvolte popis Vašeho kanálu.</translation>
    </message>
    <message>
        <source>Full feed url including scheme</source>
        <translation>Plné url kanálu včetně schématu</translation>
    </message>
    <message>
        <source>Set url for your feed.</source>
        <translation>Zvolte url Vašeho kanálu.</translation>
    </message>
    <message>
        <source>Set username to access the feed.</source>
        <translation>Nastavte uživatelské jméno pro tento kanál.</translation>
    </message>
    <message>
        <source>Set password to access the feed.</source>
        <translation>Nastavte heslo pro tento kanál.</translation>
    </message>
    <message>
        <source>Icon selection</source>
        <translation>Vybrat ikonu</translation>
    </message>
    <message>
        <source>Load icon from file...</source>
        <translation>Načíst ikonu ze souboru...</translation>
    </message>
    <message>
        <source>Do not use icon</source>
        <translation>Nepoužít ikonu</translation>
    </message>
    <message>
        <source>Use default icon</source>
        <translation>Použít výchozí ikonu</translation>
    </message>
    <message>
        <source>No metadata fetched so far.</source>
        <translation>Metadata doposud nenačtena.</translation>
    </message>
    <message>
        <source>Auto-update using global interval</source>
        <translation>Auto-aktualizovat dle hlavního nastavení</translation>
    </message>
    <message>
        <source>Auto-update every</source>
        <translation>Auto-aktualizovat každých</translation>
    </message>
    <message>
        <source>Do not auto-update at all</source>
        <translation>Zakázat auto-aktualizace</translation>
    </message>
    <message>
        <source>The description is ok.</source>
        <translation>Popis je v pořádku.</translation>
    </message>
    <message>
        <source>Feed was not edited due to error.</source>
        <translation>Kanál nebyl upraven kvůli chybě.</translation>
    </message>
</context>
<context>
    <name>FormImportExport</name>
    <message>
        <source>&amp;Select file</source>
        <translation>&amp;Zvolit soubor</translation>
    </message>
    <message>
        <source>Operation results</source>
        <translation>Výsledky operací</translation>
    </message>
    <message>
        <source>No file is selected.</source>
        <translation>Nevybrán žádný soubor.</translation>
    </message>
    <message>
        <source>No operation executed yet.</source>
        <translation>Doposud neprovedena žádná operace.</translation>
    </message>
    <message>
        <source>Export feeds</source>
        <translation>Exportovat kanály</translation>
    </message>
    <message>
        <source>Destination file</source>
        <translation>Cílový soubor</translation>
    </message>
    <message>
        <source>Source feeds &amp;&amp; categories</source>
        <translation>Zdrojové kanály &amp;&amp; kategorie</translation>
    </message>
    <message>
        <source>Source file</source>
        <translation>Zdrojový soubor</translation>
    </message>
    <message>
        <source>Target feeds &amp;&amp; categories</source>
        <translation>Cílové kanály &amp;&amp; kategorie</translation>
    </message>
    <message>
        <source>Import feeds</source>
        <translation>Importovat kanály</translation>
    </message>
    <message>
        <source>OPML 2.0 files (*.opml)</source>
        <translation>soubory OPML 2.0 (*.opml)</translation>
    </message>
    <message>
        <source>Select file for feeds export</source>
        <translation>Zvolit soubor pro export kanálů</translation>
    </message>
    <message>
        <source>File is selected.</source>
        <translation>Soubor je vybrán.</translation>
    </message>
    <message>
        <source>Select file for feeds import</source>
        <translation>Zvolit soubot pro import kanálů</translation>
    </message>
    <message>
        <source>Cannot open source file.</source>
        <translation>Zdrojový soubor nelze otevřít.</translation>
    </message>
    <message>
        <source>Feeds were loaded.</source>
        <translation>Kanály načteny.</translation>
    </message>
    <message>
        <source>Error, file is not well-formed. Select another file.</source>
        <translation>Chyba, soubor nemá správný formát, zvolte jiný.</translation>
    </message>
    <message>
        <source>Error occurred. File is not well-formed. Select another file.</source>
        <translation>Chyba, soubor nemá správný formát, zvolte jiný.</translation>
    </message>
    <message>
        <source>Feeds were exported successfully.</source>
        <translation>Kanály byly úspěšně exportovány.</translation>
    </message>
    <message>
        <source>Cannot write into destination file.</source>
        <translation>Do cílového souboru nelze zapisovat.</translation>
    </message>
    <message>
        <source>Critical error occurred.</source>
        <translation>Vyskytla se kritická chyba.</translation>
    </message>
    <message>
        <source>&amp;Check all items</source>
        <translation>&amp;Označit vše</translation>
    </message>
    <message>
        <source>&amp;Uncheck all items</source>
        <translation>O&amp;dznačit vše</translation>
    </message>
</context>
<context>
    <name>FormMain</name>
    <message>
        <source>&amp;File</source>
        <translation>&amp;Soubor</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Pomoc</translation>
    </message>
    <message>
        <source>&amp;View</source>
        <translation>&amp;Zobrazit</translation>
    </message>
    <message>
        <source>&amp;Tools</source>
        <translation>&amp;Nástroje</translation>
    </message>
    <message>
        <source>&amp;Quit</source>
        <translation>&amp;Ukončit</translation>
    </message>
    <message>
        <source>&amp;Settings</source>
        <translation>Na&amp;stavení</translation>
    </message>
    <message>
        <source>&amp;Current tab</source>
        <translation>&amp;Aktivní panel</translation>
    </message>
    <message>
        <source>&amp;Add tab</source>
        <translation>&amp;Přidat panel</translation>
    </message>
    <message>
        <source>&amp;Messages</source>
        <translation>&amp;Zprávy</translation>
    </message>
    <message>
        <source>&amp;Web browser</source>
        <translation>&amp;Webovy prohlížeč</translation>
    </message>
    <message>
        <source>Switch &amp;importance of selected messages</source>
        <translation>Přepnout &amp;důležitost vybraných zpráv</translation>
    </message>
    <message>
        <source>Quit the application.</source>
        <translation>Ukončit aplikaci.</translation>
    </message>
    <message>
        <source>Display settings of the application.</source>
        <translation>Zobrazit nastavení aplikace.</translation>
    </message>
    <message>
        <source>Switch fullscreen mode.</source>
        <translation>Přepnout režim celé obrazovky.</translation>
    </message>
    <message>
        <source>Add new web browser tab.</source>
        <translation>Přidat nový panel webového prohlížeče.</translation>
    </message>
    <message>
        <source>Close current web browser tab.</source>
        <translation>Zavřít aktuální panel webového prohlížeče..</translation>
    </message>
    <message>
        <source>No actions available</source>
        <translation>Žádná dostupná akce</translation>
    </message>
    <message>
        <source>No actions are available right now.</source>
        <translation>Žádná akce není právě dostupná.</translation>
    </message>
    <message>
        <source>Fee&amp;ds &amp;&amp; categories</source>
        <translation>Kanály &amp;&amp; ka&amp;tegorie</translation>
    </message>
    <message>
        <source>Mark all messages (without message filters) from selected feeds as read.</source>
        <translation>Označit všechny zprávy (i přes filtry zpráv) z vybraných kanálů jako přečtené.</translation>
    </message>
    <message>
        <source>Mark all messages (without message filters) from selected feeds as unread.</source>
        <translation>Označit všechny zprávy (i přes filtry zpráv) z vybraných kanálů jako nepřečtené.</translation>
    </message>
    <message>
        <source>Displays all messages from selected feeds/categories in a new &quot;newspaper mode&quot; tab. Note that messages are not set as read automatically.</source>
        <translation>Zobrazí všechny zprávy z vybraných kanálů/kategorií v &quot;novinovém&quot; náhledu. Všechny zprávy budou automaticky označeny jako přečtené.</translation>
    </message>
    <message>
        <source>Hides main window if it is visible and shows it if it is hidden.</source>
        <translation>Skryje hlavní ikno, je-li aktuálně viditelné. Jinak jej zobrazí.</translation>
    </message>
    <message>
        <source>Defragment database file so that its size decreases.</source>
        <translation>Defragmentuje a optimalizuje databázi.</translation>
    </message>
    <message>
        <source>Hides or shows the list of feeds/categories.</source>
        <translation>Skryje nebo zobrazí seznam kanálů/kategorií.</translation>
    </message>
    <message>
        <source>Check if new update for the application is available for download.</source>
        <translation>Zkontrolovat, zda nejsou k dispozici aktualizace programu.</translation>
    </message>
    <message>
        <source>Cannot check for updates</source>
        <translation>Nelze zkontrolovat</translation>
    </message>
    <message>
        <source>You cannot check for updates because feed update is ongoing.</source>
        <translation>Nelze spustit kontrolu aktualizací, protože běží aktualizace kanálů.</translation>
    </message>
    <message>
        <source>&amp;About application</source>
        <translation>O &amp;aplikaci</translation>
    </message>
    <message>
        <source>Displays extra info about this application.</source>
        <translation>Zobrazí dodatečné informace o této aplikaci.</translation>
    </message>
    <message>
        <source>&amp;Delete selected messages</source>
        <translation>Sma&amp;zat vybrané zprávy</translation>
    </message>
    <message>
        <source>Deletes all messages from selected feeds.</source>
        <translation>Smaže všechny zprávy z vybraných kanálů.</translation>
    </message>
    <message>
        <source>Marks all messages in all feeds read. This does not take message filters into account.</source>
        <translation>Označí všechny zprávy ve všech kanálech jako přečtené. Tato funkce nemusí brát v potaz případně filtry zpráv.</translation>
    </message>
    <message>
        <source>Deletes all messages from all feeds.</source>
        <translation>Smaže všechny zprávy ze všech kanálů.</translation>
    </message>
    <message>
        <source>Update &amp;all feeds</source>
        <translation>Aktualizovat všechny k&amp;anály</translation>
    </message>
    <message>
        <source>Update &amp;selected feeds</source>
        <translation>Aktualizovat vy&amp;brané kanály</translation>
    </message>
    <message>
        <source>&amp;Edit selected feed/category</source>
        <translation>Up&amp;ravit vybraný kanál/kategorii</translation>
    </message>
    <message>
        <source>&amp;Delete selected feed/category</source>
        <translation>Smazat vybraný kaná&amp;l/kategorii</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Nastavení</translation>
    </message>
    <message>
        <source>Hides or displays the main menu.</source>
        <translation>Skryje či zobrazí hlavní menu.</translation>
    </message>
    <message>
        <source>Add &amp;new feed/category</source>
        <translation>&amp;Přidat novou položku</translation>
    </message>
    <message>
        <source>&amp;Close all tabs except current one</source>
        <translation>&amp;Zavřít všechny taby až na ten aktivní</translation>
    </message>
    <message>
        <source>&amp;Close current tab</source>
        <translation>&amp;Zavřít aktivní tab</translation>
    </message>
    <message>
        <source>Mark &amp;selected messages as &amp;read</source>
        <translation>Označit vybrané zprávy jako &amp;přečtené</translation>
    </message>
    <message>
        <source>Mark &amp;selected messages as &amp;unread</source>
        <translation>Označit vybrané zprávy jako &amp;nepřečtené</translation>
    </message>
    <message>
        <source>&amp;Mark selected feeds as read</source>
        <translation>Označit vybrané kanály jako &amp;přečtené</translation>
    </message>
    <message>
        <source>&amp;Mark selected feeds as unread</source>
        <translation>Označit vybrané kanály jako &amp;nepřečtené</translation>
    </message>
    <message>
        <source>&amp;Clean selected feeds</source>
        <translation>&amp;Vyčistit vybrané kanály</translation>
    </message>
    <message>
        <source>Open selected source articles in &amp;external browser</source>
        <translation>&amp;Otevřít vybrané zdrojové články v externím prohlížeči</translation>
    </message>
    <message>
        <source>Open selected messages in &amp;internal browser</source>
        <translation>&amp;Otevřít vybrané krátké články v interním prohlížeči</translation>
    </message>
    <message>
        <source>Open selected source articles in &amp;internal browser</source>
        <translation>&amp;Otevřít vybrané zdrojové články v interním prohlížeči</translation>
    </message>
    <message>
        <source>&amp;Mark all feeds as &amp;read</source>
        <translation>Označit všechny kanály jako &amp;přečtené</translation>
    </message>
    <message>
        <source>View selected feeds in &amp;newspaper mode</source>
        <translation>Zobrazit vybrané kanály v &amp;novinovém náhledu</translation>
    </message>
    <message>
        <source>&amp;Defragment database</source>
        <translation>&amp;Optimalizovat databázi</translation>
    </message>
    <message>
        <source>&amp;Clean all feeds</source>
        <translation>&amp;Vyčistit všechny kanály</translation>
    </message>
    <message>
        <source>Select &amp;next feed/category</source>
        <translation>Vybrat &amp;další kanál/kategorii</translation>
    </message>
    <message>
        <source>Select &amp;previous feed/category</source>
        <translation>Vybrat &amp;předchozí kanál/kategorii</translation>
    </message>
    <message>
        <source>Select &amp;next message</source>
        <translation>Vybrat &amp;další zprávu</translation>
    </message>
    <message>
        <source>Select &amp;previous message</source>
        <translation>Vybrat &amp;předchozí zprávu</translation>
    </message>
    <message>
        <source>Check for &amp;updates</source>
        <translation>Ověřit dostupnost &amp;aktualizace</translation>
    </message>
    <message>
        <source>Enable &amp;JavaScript</source>
        <translation>Povolit &amp;JavaScript</translation>
    </message>
    <message>
        <source>Enable external &amp;plugins</source>
        <translation>Povolit externí &amp;pluginy</translation>
    </message>
    <message>
        <source>Auto-load &amp;images</source>
        <translation>Automaticky načítat &amp;obrázky</translation>
    </message>
    <message>
        <source>Show/hide</source>
        <translation>Zobrazit/skrýt</translation>
    </message>
    <message>
        <source>&amp;Fullscreen</source>
        <translation>Přes celou &amp;obrazovku</translation>
    </message>
    <message>
        <source>&amp;Feed list</source>
        <translation>Seznam &amp;kanálů</translation>
    </message>
    <message>
        <source>&amp;Main menu</source>
        <translation>Hlavní &amp;menu</translation>
    </message>
    <message>
        <source>Switch visibility of main &amp;window</source>
        <translation>Přepnout &amp;hlavní okno</translation>
    </message>
    <message>
        <source>Cannot open external browser</source>
        <translation>Nelze otevřít externí prohlížeč webu</translation>
    </message>
    <message>
        <source>Cannot open external browser. Navigate to application website manually.</source>
        <translation>Externí webový prohlížeč nelze otevřít. Zkontrolujte aktualizace ručně na webu programu.</translation>
    </message>
    <message>
        <source>New &amp;feed</source>
        <translation>Nový &amp;kanál</translation>
    </message>
    <message>
        <source>Add new feed.</source>
        <translation>Přidat nový kanál.</translation>
    </message>
    <message>
        <source>New &amp;category</source>
        <translation>No&amp;vá kategorie</translation>
    </message>
    <message>
        <source>Add new category.</source>
        <translation>Přidat novou kategorii.</translation>
    </message>
    <message>
        <source>&amp;Toolbars</source>
        <translation>&amp;Nástrojové lišty</translation>
    </message>
    <message>
        <source>Switch visibility of main toolbars.</source>
        <translation>Přepnout viditelnost hlavnich nástrojových lišet.</translation>
    </message>
    <message>
        <source>&amp;Feed/message list headers</source>
        <translation>&amp;Hlavičky seznamů zpráv/kanálů</translation>
    </message>
    <message>
        <source>&amp;Import feeds</source>
        <translation>&amp;Importovat kanály</translation>
    </message>
    <message>
        <source>Imports feeds you want from selected file.</source>
        <translation>Importuje kanály ze souboru.</translation>
    </message>
    <message>
        <source>&amp;Export feeds</source>
        <translation>&amp;Exportovat kanály</translation>
    </message>
    <message>
        <source>Exports feeds you want to selected file.</source>
        <translation>Exportuje kanály do souboru.</translation>
    </message>
    <message>
        <source>Close all tabs except current one.</source>
        <translation>Zavřít všechny taby kromě aktivního.</translation>
    </message>
    <message>
        <source>&amp;Recycle bin</source>
        <translation>&amp;Koš</translation>
    </message>
    <message>
        <source>Report a &amp;bug (GitHub)...</source>
        <translation>Nahlásit &amp;chybu (GitHub)...</translation>
    </message>
    <message>
        <source>Report a bug (BitBucket)...</source>
        <translation>Nahlásit chybu (BitBucket)...</translation>
    </message>
    <message>
        <source>&amp;Donate via PayPal</source>
        <translation>&amp;Podpořit vývojáře přes PayPal</translation>
    </message>
    <message>
        <source>Display &amp;wiki</source>
        <translation>Zobrazit &amp;wiki</translation>
    </message>
    <message>
        <source>&amp;Empty recycle bin</source>
        <translation>&amp;Vysypat koš</translation>
    </message>
    <message>
        <source>&amp;Restore all messages</source>
        <translation>&amp;Obnovit všechny zprávy z koše</translation>
    </message>
    <message>
        <source>Restore &amp;selected messages</source>
        <translation>Obnovit &amp;vybrané zprávy z koše</translation>
    </message>
    <message>
        <source>&amp;Restart</source>
        <translation>&amp;Restartovat</translation>
    </message>
</context>
<context>
    <name>FormSettings</name>
    <message>
        <source>General</source>
        <extracomment>General settings section.</extracomment>
        <translation>Obecné</translation>
    </message>
    <message>
        <source>User interface</source>
        <translation>Uživatelské rozhraní</translation>
    </message>
    <message>
        <source>Icon theme</source>
        <translation>Téma ikon</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Nastavení</translation>
    </message>
    <message>
        <source>Keyboard shortcuts</source>
        <translation>Klávesové zkratky</translation>
    </message>
    <message>
        <source>Language</source>
        <extracomment>Language settings section.
----------
Language column of language list.</extracomment>
        <translation>Lokalizace</translation>
    </message>
    <message>
        <source>Proxy</source>
        <translation></translation>
    </message>
    <message>
        <source>Icons &amp;&amp; skins</source>
        <translation>Ikony &amp;&amp; skiny</translation>
    </message>
    <message>
        <source>Tray icon</source>
        <translation>Notifikační ikona</translation>
    </message>
    <message>
        <source>Start application hidden</source>
        <translation>Spouštět aplikaci skrytou</translation>
    </message>
    <message>
        <source>Type</source>
        <extracomment>Proxy server type.</extracomment>
        <translation>Typ</translation>
    </message>
    <message>
        <source>Host</source>
        <translation>Server</translation>
    </message>
    <message>
        <source>Hostname or IP of your proxy server</source>
        <translation>Webový název IP adresa proxy serveru</translation>
    </message>
    <message>
        <source>Port</source>
        <translation></translation>
    </message>
    <message>
        <source>Username</source>
        <translation>Uživatelské jméno</translation>
    </message>
    <message>
        <source>Your username for proxy server authentication</source>
        <translation>Vaše uživatelské jméno pro autentifikaci</translation>
    </message>
    <message>
        <source>Password</source>
        <translation>Heslo</translation>
    </message>
    <message>
        <source>Your password for proxy server authentication</source>
        <translation>Vaše heslo pro autentifikaci</translation>
    </message>
    <message>
        <source>Display password</source>
        <translation>Zobrazit heslo</translation>
    </message>
    <message>
        <source>Code</source>
        <extracomment>Lang. code column of language list.</extracomment>
        <translation>Kód</translation>
    </message>
    <message>
        <source>Version</source>
        <extracomment>Version column of skin list.</extracomment>
        <translation>Verze</translation>
    </message>
    <message>
        <source>Author</source>
        <translation>Autor</translation>
    </message>
    <message>
        <source>Email</source>
        <translation></translation>
    </message>
    <message>
        <source>Socks5</source>
        <translation></translation>
    </message>
    <message>
        <source>Http</source>
        <translation></translation>
    </message>
    <message>
        <source> (not supported on this platform)</source>
        <translation> (na této platformě nepodporováno)</translation>
    </message>
    <message>
        <source>Tray area &amp;&amp; notifications</source>
        <translation>Notifikační oblast</translation>
    </message>
    <message>
        <source>Disable</source>
        <translation>Zakázat</translation>
    </message>
    <message>
        <source>Enable</source>
        <translation>Povolit</translation>
    </message>
    <message>
        <source>Tabs</source>
        <translation>Panely</translation>
    </message>
    <message>
        <source>Close tabs with</source>
        <translation>Zavírat panely pomocí</translation>
    </message>
    <message>
        <source>Middle mouse button single-click</source>
        <translation>Prostředního tlačítka myši</translation>
    </message>
    <message>
        <source>Open new tabs with left mouse button double-click on tab bar</source>
        <translation>Otevírat nové panely poklepáním na panelový pruh</translation>
    </message>
    <message>
        <source>Enable mouse gestures</source>
        <translation>Povolit gesta myši</translation>
    </message>
    <message>
        <source>Web browser &amp; proxy</source>
        <translation>Webový prohlížeč &amp; proxy</translation>
    </message>
    <message>
        <source>Disable (Tray icon is not available.)</source>
        <translation>Zakázat (Notifikační ikona není k dispozici.)</translation>
    </message>
    <message>
        <source>Queue new tabs (with hyperlinks) after the active tab</source>
        <translation>Zařadit nově otevíraný panel za ten aktuální</translation>
    </message>
    <message>
        <source>no icon theme</source>
        <extracomment>Label for disabling icon theme.</extracomment>
        <translation>žádné téma ikon</translation>
    </message>
    <message>
        <source>Cannot save settings</source>
        <translation>Nastavení nelze uložit</translation>
    </message>
    <message>
        <source>Name</source>
        <extracomment>Skin list name column.</extracomment>
        <translation>Název</translation>
    </message>
    <message>
        <source>Icons</source>
        <translation>Ikony</translation>
    </message>
    <message>
        <source>Skins</source>
        <translation>Skiny</translation>
    </message>
    <message>
        <source>Active skin:</source>
        <translation>Aktivní skin:</translation>
    </message>
    <message>
        <source>Selected skin:</source>
        <translation>Vybraný skin:</translation>
    </message>
    <message>
        <source>Hide tab bar if just one tab is visible</source>
        <translation>Skrýt přepínač tabů, je-li viditelný pouze jeden tab</translation>
    </message>
    <message>
        <source>Critical settings were changed</source>
        <translation>Kritická nastavení změněna</translation>
    </message>
    <message>
        <source>Feeds &amp; messages</source>
        <translation>Kanály &amp; zprávy</translation>
    </message>
    <message>
        <source>Some critical settings are not set. You must fix these settings in order confirm new settings.</source>
        <translation>Některá kritická nastavení nejsou vyplněna. Musíte je opravit než bude možné dialog potvrdit.</translation>
    </message>
    <message>
        <source>Messages</source>
        <translation>Zprávy</translation>
    </message>
    <message>
        <source>Web browser executable</source>
        <translation>Spouštěcí soubor</translation>
    </message>
    <message>
        <source>...</source>
        <translation></translation>
    </message>
    <message>
        <source>Executable parameters</source>
        <translation>Parametry spouštěče</translation>
    </message>
    <message>
        <source>Note that &quot;%1&quot; (without quotation marks) is placeholder for URL of selected message.</source>
        <translation>Mějte na vědomí, že &quot;%1&quot; (bez uvozovek) je zástupný symbol pro URL dané zprávy.</translation>
    </message>
    <message>
        <source>Select web browser executable</source>
        <translation>Zvolit spouštěč webového externího prohlížeče</translation>
    </message>
    <message>
        <source>Executables (*.*)</source>
        <extracomment>File filter for external browser selection dialog.</extracomment>
        <translation>Spouštěče (*.*)</translation>
    </message>
    <message>
        <source>Opera 12 or older</source>
        <translation>Opera 12 nebo starší</translation>
    </message>
    <message>
        <source>Executable file of web browser</source>
        <translation>Spouštěcí soubor prohlžeče webu</translation>
    </message>
    <message>
        <source>Parameters to executable</source>
        <translation>Parametry spouštěče</translation>
    </message>
    <message>
        <source>some keyboard shortcuts are not unique</source>
        <translation>některé klávesové zkratky nejsou unikátní</translation>
    </message>
    <message>
        <source>List of errors:
%1.</source>
        <translation>Seznam chyb:
%1.</translation>
    </message>
    <message>
        <source>List of changes:
%1.</source>
        <translation>Seznam změn:
%1.</translation>
    </message>
    <message>
        <source>language changed</source>
        <translation>jazyk změněn</translation>
    </message>
    <message>
        <source>icon theme changed</source>
        <translation>téma ikon změněno</translation>
    </message>
    <message>
        <source>skin changed</source>
        <translation>skin změněn</translation>
    </message>
    <message>
        <source>Use sample arguments for</source>
        <translation>Použít typické argumenty pro</translation>
    </message>
    <message>
        <source>Use in-memory database as the working database</source>
        <translation>Použít paměťovou datábázi jako pracovní datové uložiště</translation>
    </message>
    <message>
        <source>Usage of in-memory working database has several advantages and pitfalls. Make sure that you are familiar with these before you turn this feature on. Advantages:
&lt;ul&gt;
&lt;li&gt;higher speed for feed/message manipulations (especially with thousands of messages displayed),&lt;/li&gt;
&lt;li&gt;whole database stored in RAM, thus your hard drive can rest more.&lt;/li&gt;
&lt;/ul&gt;
Disadvantages:
&lt;ul&gt;
&lt;li&gt;if application crashes, your changes from last session are lost,&lt;/li&gt;
&lt;li&gt;application startup and shutdown can take little longer (max. 2 seconds).&lt;/li&gt;
&lt;/ul&gt;
Authors of this application are NOT responsible for lost data.</source>
        <translation>Použití paměťové databáze má hned několik výhod a nevýhod. Před povolením této funkce se ujistětě, že se s nimi seznámíte. Výhody:
&lt;ul&gt;
&lt;li&gt;vyšší rychlost při manipulaci se zprávami a kanály (obzvláště, pokud je zpráv hodně),&lt;/li&gt;
&lt;li&gt;celá databáze je uložená v RAM, tedy pevný disk může více odpočívat.&lt;/li&gt;
&lt;/ul&gt;
Nevýhody:
&lt;ul&gt;
&lt;li&gt;pokud aplikace zhavaruje, tak Vaše změny z posledního sezení budou ztraceny,&lt;/li&gt;
&lt;li&gt;start a vypnutí aplikace může trvat o něco déle (max. 2 vteřiny).&lt;/li&gt;
&lt;/ul&gt;
Autoři této aplikace nenesou žádnou odpovědnost za ztrátu Vašich dat.</translation>
    </message>
    <message>
        <source>in-memory database switched</source>
        <translation>paměťová databáze přepnuta</translation>
    </message>
    <message>
        <source>Internal web browser</source>
        <translation>Interní webový prohlížeč</translation>
    </message>
    <message>
        <source>External web browser</source>
        <translation>Externí webový prohlížeč</translation>
    </message>
    <message>
        <source>Remove all read messages from all standard feeds on application exit</source>
        <translation>Smazat všechny přečtené zprávy ze všech kanálů při vypnutí aplikace</translation>
    </message>
    <message>
        <source>WARNING: Note that switching to another data storage type will NOT copy existing your data from currently active data storage to newly selected one.</source>
        <translation>VAROVÁNÍ: Mějte na paměti, že přepnutí z jednoho databázového backendu na jiný neprovede zkopírování dat mezi těmito backendy.</translation>
    </message>
    <message>
        <source>Database driver</source>
        <translation>Datbázový ovladač</translation>
    </message>
    <message>
        <source>Hostname</source>
        <translation>Hostitel</translation>
    </message>
    <message>
        <source>Test setup</source>
        <translation>Otestovat</translation>
    </message>
    <message>
        <source>Note that speed of used MySQL server and latency of used connection medium HEAVILY influences the final performance of this application. Using slow database connections leads to bad performance when browsing feeds or messages.</source>
        <translation>Berte na vědomí, že latence a celková rychlost zvoleného MySQL serveru může mít rozhodující vliv na rychlost této aplikace. Použití pomalého serveru může vést k tomu, že práce se zprávami či kanály bude neúměrně pomalá.</translation>
    </message>
    <message>
        <source>Right mouse button double-click</source>
        <translation>Dvojklik pravého tlačítka myši</translation>
    </message>
    <message>
        <source>Auto-update all feeds every</source>
        <translation>Auto-aktualizovat všechny kanály každých</translation>
    </message>
    <message>
        <source> minutes</source>
        <translation> minut</translation>
    </message>
    <message>
        <source>Feed connection timeout</source>
        <translation>Časový limit stažení souboru kanálu</translation>
    </message>
    <message>
        <source>Connection timeout is time interval which is reserved for downloading new messages for the feed. If this time interval elapses, then download process is aborted.</source>
        <translation>Tento časový limit označuje čas, který si stahovací komponenta vyhradí pro získání souboru každého kanálu. Pokud tento čas vyprší a soubor doposud nebyl získán, tak se aktualizace kanálu ukončí a přejde se na další kanál.</translation>
    </message>
    <message>
        <source> ms</source>
        <translation></translation>
    </message>
    <message>
        <source>Update all feed on application startup</source>
        <translation>Aktualizovat všechny kanály po startu aplikace</translation>
    </message>
    <message>
        <source>Data storage</source>
        <translation>Uložiště dat</translation>
    </message>
    <message>
        <source>SQLite (embedded database)</source>
        <translation>SQLite (embedded databáze)</translation>
    </message>
    <message>
        <source>MySQL/MariaDB (dedicated database)</source>
        <translation>MySQL/MariaDB (dedikovaná database)</translation>
    </message>
    <message>
        <source>Hostname of your MySQL server</source>
        <translation>Hostitel Vašeho MySQL serveru</translation>
    </message>
    <message>
        <source>Username to login with</source>
        <translation>Uživatelské jméno</translation>
    </message>
    <message>
        <source>Password for your username</source>
        <translation>Heslo</translation>
    </message>
    <message>
        <source>data storage backend changed</source>
        <translation>databázový backend změněn</translation>
    </message>
    <message>
        <source>Hostname is empty.</source>
        <translation>Hostitel je prázdný.</translation>
    </message>
    <message>
        <source>Hostname looks ok.</source>
        <translation>Hostitel vypadá být v pořádku.</translation>
    </message>
    <message>
        <source>Username is empty.</source>
        <translation>Uživatelské jméno je prázdné.</translation>
    </message>
    <message>
        <source>Username looks ok.</source>
        <translation>Uživatelské jméno se jeví být v pořádku.</translation>
    </message>
    <message>
        <source>Password is empty.</source>
        <translation>Heslo je prázdné.</translation>
    </message>
    <message>
        <source>Password looks ok.</source>
        <translation>Heslo vypadá dobře.</translation>
    </message>
    <message>
        <source>Toolbar button style</source>
        <translation>Styl nástrojové lišty</translation>
    </message>
    <message>
        <source>Hide main window when it is minimized</source>
        <translation>Skrýt hlavní okno při jeho minimalizaci</translation>
    </message>
    <message>
        <source>No connection test triggered so far.</source>
        <translation>Test připojení doposud nespuštěn.</translation>
    </message>
    <message>
        <source>Note that these settings are applied only on newly established connections.</source>
        <translation>Tato nastavení se projeví pouze na nově vytvořených spojeních.</translation>
    </message>
    <message>
        <source>Select browser</source>
        <translation>Zvolte prohlížeč</translation>
    </message>
    <message>
        <source>No proxy</source>
        <translation>Bez proxy</translation>
    </message>
    <message>
        <source>System proxy</source>
        <translation>Systémová proxy</translation>
    </message>
    <message>
        <source>Icon only</source>
        <translation>Pouze ikony</translation>
    </message>
    <message>
        <source>Text only</source>
        <translation>Pouze texty</translation>
    </message>
    <message>
        <source>Text beside icon</source>
        <translation>Texty vedle ikon</translation>
    </message>
    <message>
        <source>Text under icon</source>
        <translation>Texty pod ikonami</translation>
    </message>
    <message>
        <source>Follow OS style</source>
        <translation>Podle nastavení systému</translation>
    </message>
    <message>
        <source>Keep message selection in the middle of the message list viewport</source>
        <translation>Držet vybranou zprávu uprostřed seznamu zpráv při jejich procházení</translation>
    </message>
    <message>
        <source>You did not executed any connection test yet.</source>
        <translation>Doposud nebyl probeden žádny test spojení.</translation>
    </message>
    <message>
        <source>Launch %1 on operating system startup</source>
        <translation>Spustit %1 při spuštění operačního systému</translation>
    </message>
    <message>
        <source>Mouse gestures work with middle mouse button. Possible gestures are:
&lt;ul&gt;
&lt;li&gt;previous web page (drag mouse left),&lt;/li&gt;
&lt;li&gt;next web page (drag mouse right),&lt;/li&gt;
&lt;li&gt;reload current web page (drag mouse up),&lt;/li&gt;
&lt;li&gt;open new web browser tab (drag mouse down).&lt;/li&gt;
&lt;/ul&gt;</source>
        <translation>Gesta myši se používají středním tlačítkem myši. Dostupná gesta:
&lt;ul&gt;
&lt;li&gt;předchozí stránka (táhnutí myši vlevo),&lt;/li&gt;
&lt;li&gt;následující stránka (táhnutí myši vpravo),&lt;/li&gt;
&lt;li&gt;znovu načíst stránku (táhnutí myši nahoru),&lt;/li&gt;
&lt;li&gt;otevřít nový tab webového prohlížeče (táhnutí myši dolů).&lt;/li&gt;
&lt;/ul&gt;</translation>
    </message>
    <message>
        <source>Enable JavaScript</source>
        <translation>Povolit JavaScript</translation>
    </message>
    <message>
        <source>Enable external plugins based on NPAPI</source>
        <translation>Povolit externí NPAPI pluginy</translation>
    </message>
    <message>
        <source>Auto-load images</source>
        <translation>Načítat obrázky ve webových stránkách</translation>
    </message>
    <message>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If unchecked, then default system-wide web browser is used.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Pokud bude nezaškrtnuto, pak se použije výchozí systémový webový prohlížeč.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <source>Custom external web browser</source>
        <translation>Vlastní externí webový prohlížeč</translation>
    </message>
    <message>
        <source>Feeds &amp;&amp; categories</source>
        <translation>Kanály &amp;&amp; kategorie</translation>
    </message>
    <message>
        <source>Message count format in feed list</source>
        <translation>Formát čítače zpráv v seznamu kanálů</translation>
    </message>
    <message>
        <source>Enter format for count of messages displayed next to each feed/category in feed list. Use &quot;%all&quot; and &quot;%unread&quot; strings which are placeholders for the actual count of all (or unread) messages.</source>
        <translation>Zadejte formát pro čítače zpráv, které jsou zobrazeny u každého kanálu/kategorie. Použijte zástupné symboly &quot;%all&quot; a &quot;%unread&quot;, které představují aktuální hodnoty čítačů.</translation>
    </message>
    <message>
        <source>custom external browser is not set correctly</source>
        <translation>vlastní externí prohlížeč není správně nastaven</translation>
    </message>
    <message>
        <source>Toolbars</source>
        <translation>Nástrojové lišty</translation>
    </message>
    <message>
        <source>Toolbar for feeds list</source>
        <translation>Lišta seznamu kanálů</translation>
    </message>
    <message>
        <source>Toolbar for messages list</source>
        <translation>Lišta seznamu zpráv</translation>
    </message>
    <message>
        <source>Select toolbar to edit</source>
        <translation>Zvolte lištu, kterou chcete upravit</translation>
    </message>
    <message>
        <source>Some critical settings were changed and will be applied after the application gets restarted. 

You have to restart manually.</source>
        <translation>Některá kritická nastavení se změnila a budou aktivována až po restartu aplikace.

Musíte restartovat manuálně.</translation>
    </message>
    <message>
        <source>Do you want to restart now?</source>
        <translation>Chcete restartovat nyní?</translation>
    </message>
</context>
<context>
    <name>FormUpdate</name>
    <message>
        <source>Current release</source>
        <translation>Nainstalovaná verze</translation>
    </message>
    <message>
        <source>Available release</source>
        <translation>Dostupná verze</translation>
    </message>
    <message>
        <source>Changes</source>
        <translation>Změny</translation>
    </message>
    <message>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <source>unknown</source>
        <extracomment>Unknown release.</extracomment>
        <translation>neznámá</translation>
    </message>
    <message>
        <source>List with updates was not
downloaded successfully.</source>
        <translation>Seznam aktualizací nebyl úspěšně stažen.</translation>
    </message>
    <message>
        <source>New release available.</source>
        <translation>Nová verze aplikace je dostupná.</translation>
    </message>
    <message>
        <source>This is new version which can be
downloaded and installed.</source>
        <translation>Toto je nová verze, která může být stažena a nainstalována.</translation>
    </message>
    <message>
        <source>Error: &apos;%1&apos;.</source>
        <translation>Chyba: &apos;%1&apos;.</translation>
    </message>
    <message>
        <source>No new release available.</source>
        <translation>Žádná nová verze k dispozici.</translation>
    </message>
    <message>
        <source>This release is not newer than
currently installed one.</source>
        <translation>Toto vydání není novější než aktuálně nainstalované.</translation>
    </message>
    <message>
        <source>Check for updates</source>
        <translation>Zkontrolovat aktualizace</translation>
    </message>
    <message>
        <source>Update</source>
        <translation>Aktualizovat</translation>
    </message>
    <message>
        <source>Download new installation files.</source>
        <translation>Stáhnout instalační soubory nové verze.</translation>
    </message>
    <message>
        <source>Checking for updates failed.</source>
        <translation>Kontrola aktualizací selhala.</translation>
    </message>
    <message>
        <source>Download installation file for your OS.</source>
        <translation>Stáhnout instalační soubor pro Váš OS.</translation>
    </message>
    <message>
        <source>Installation file is not available directly.
Go to application website to obtain it manually.</source>
        <translation>Instalační soubor není přímo dostupný.
Přejít na web aplikace a stáhnout jej ručně.</translation>
    </message>
    <message>
        <source>No new update available.</source>
        <translation>Žádná nová aktualizace není k dispozici.</translation>
    </message>
    <message>
        <source>Cannot update application</source>
        <translation>Nelze aktualizovat</translation>
    </message>
    <message>
        <source>Cannot navigate to installation file. Check new installation downloads manually on project website.</source>
        <translation>Nelze přejít k instalačnímu souboru. Zkontrolujte nové instalační soubory ručně na webu aplikace.</translation>
    </message>
    <message>
        <source>Download update</source>
        <translation>Stáhnout aktualizaci</translation>
    </message>
    <message>
        <source>Downloaded %1% (update size is %2 kB).</source>
        <translation>Staženo %1% (velikost aktualizace je %2 kB).</translation>
    </message>
    <message>
        <source>Downloading update...</source>
        <translation>Stahuji aktualizaci...</translation>
    </message>
    <message>
        <source>Downloaded successfully</source>
        <translation>Staženo úspěšně</translation>
    </message>
    <message>
        <source>Package was downloaded successfully.</source>
        <translation>Balík aktualizace úspěšně stažen.</translation>
    </message>
    <message>
        <source>Install update</source>
        <translation>Instalovat aktualizaci</translation>
    </message>
    <message>
        <source>Error occured</source>
        <translation>Vyskytla se chyba</translation>
    </message>
    <message>
        <source>Error occured during downloading of the package.</source>
        <translation>Během stahování aktualizace se vyskytla chyba.</translation>
    </message>
    <message>
        <source>Cannot launch external updater. Update application manually.</source>
        <translation>Nelze spustit externí aktualizátor. Aktualizuje aplikaci manuálně.</translation>
    </message>
    <message>
        <source>Go to application website</source>
        <translation>Přejít na web aplikace</translation>
    </message>
</context>
<context>
    <name>LocationLineEdit</name>
    <message>
        <source>Website address goes here</source>
        <translation>Adresu webové stránky zadejte sem</translation>
    </message>
</context>
<context>
    <name>MessagesModel</name>
    <message>
        <source>Id</source>
        <extracomment>Tooltip for ID of message.</extracomment>
        <translation></translation>
    </message>
    <message>
        <source>Read</source>
        <extracomment>Tooltip for &quot;read&quot; column in msg list.</extracomment>
        <translation>Přečteno</translation>
    </message>
    <message>
        <source>Deleted</source>
        <extracomment>Tooltip for &quot;deleted&quot; column in msg list.</extracomment>
        <translation>Smazáno</translation>
    </message>
    <message>
        <source>Important</source>
        <extracomment>Tooltip for &quot;important&quot; column in msg list.</extracomment>
        <translation>Důležité</translation>
    </message>
    <message>
        <source>Feed</source>
        <extracomment>Tooltip for name of feed for message.</extracomment>
        <translation>Kanál</translation>
    </message>
    <message>
        <source>Title</source>
        <extracomment>Tooltip for title of message.</extracomment>
        <translation>Nadpis</translation>
    </message>
    <message>
        <source>Url</source>
        <extracomment>Tooltip for url of message.</extracomment>
        <translation></translation>
    </message>
    <message>
        <source>Author</source>
        <extracomment>Tooltip for author of message.</extracomment>
        <translation>Autor</translation>
    </message>
    <message>
        <source>Created on</source>
        <extracomment>Tooltip for creation date of message.</extracomment>
        <translation>Vytvořeno</translation>
    </message>
    <message>
        <source>Contents</source>
        <extracomment>Tooltip for contents of message.</extracomment>
        <translation>Obsah</translation>
    </message>
    <message>
        <source>Id of the message.</source>
        <translation>Id zprávy.</translation>
    </message>
    <message>
        <source>Is message read?</source>
        <translation>Je zpráva přečtená?</translation>
    </message>
    <message>
        <source>Is message deleted?</source>
        <translation>Je zpráva smazaná?</translation>
    </message>
    <message>
        <source>Is message important?</source>
        <translation>Je zpráva důležitá?</translation>
    </message>
    <message>
        <source>Id of feed which this message belongs to.</source>
        <translation>Id kanálu, ke kterému zpráva náleží.</translation>
    </message>
    <message>
        <source>Title of the message.</source>
        <translation>Nadpis zprávy.</translation>
    </message>
    <message>
        <source>Url of the message.</source>
        <translation>Url zprávy.</translation>
    </message>
    <message>
        <source>Author of the message.</source>
        <translation>Autor zprávy.</translation>
    </message>
    <message>
        <source>Creation date of the message.</source>
        <translation>Datum vytvoření zprávy.</translation>
    </message>
    <message>
        <source>Contents of the message.</source>
        <translation>Obsah zprávy.</translation>
    </message>
</context>
<context>
    <name>MessagesToolBar</name>
    <message>
        <source>Search messages</source>
        <translation>Hledat zprávy</translation>
    </message>
    <message>
        <source>Message search box</source>
        <translation>Hledací panel zpráv</translation>
    </message>
    <message>
        <source>Menu for highlighting messages</source>
        <translation>Menu pro zvýrazňování zpráv</translation>
    </message>
    <message>
        <source>No extra highlighting</source>
        <translation>Nic nezvýrazňovat</translation>
    </message>
    <message>
        <source>Highlight unread messages</source>
        <translation>Zvýraznit nepřečtené zprávy</translation>
    </message>
    <message>
        <source>Highlight important messages</source>
        <translation>Zvýraznit důležité zprávy</translation>
    </message>
    <message>
        <source>Display all messages</source>
        <translation>Zobrazit všechny zprávy</translation>
    </message>
    <message>
        <source>Message highlighter</source>
        <translation>Zvýrazňovač zpráv</translation>
    </message>
    <message>
        <source>Toolbar spacer</source>
        <translation>Mezera</translation>
    </message>
</context>
<context>
    <name>MessagesView</name>
    <message>
        <source>Context menu for messages</source>
        <translation>Kontextové menu pro zprávy</translation>
    </message>
    <message>
        <source>Meesage without URL</source>
        <translation>Zpráva bez URL</translation>
    </message>
    <message>
        <source>Message &apos;%s&apos; does not contain URL.</source>
        <translation>Zpráva &apos;%s&apos; neobsahuje URL.</translation>
    </message>
    <message>
        <source>Problem with starting external web browser</source>
        <translation>PRoblém s externím webovým prohlížečem</translation>
    </message>
    <message>
        <source>External web browser could not be started.</source>
        <translation>Externí webový prohlížeč nebyl úspěšně spuštěn.</translation>
    </message>
</context>
<context>
    <name>NetworkFactory</name>
    <message>
        <source>protocol error</source>
        <extracomment>Network status.</extracomment>
        <translation>chyba protokolu</translation>
    </message>
    <message>
        <source>host not found</source>
        <extracomment>Network status.</extracomment>
        <translation>hostitel nenalezen</translation>
    </message>
    <message>
        <source>connection refused</source>
        <extracomment>Network status.</extracomment>
        <translation>spojení odmítnuto</translation>
    </message>
    <message>
        <source>connection timed out</source>
        <extracomment>Network status.</extracomment>
        <translation>spojení vypršelo</translation>
    </message>
    <message>
        <source>SSL handshake failed</source>
        <extracomment>Network status.</extracomment>
        <translation>SSL handshake selhal</translation>
    </message>
    <message>
        <source>proxy server connection refused</source>
        <extracomment>Network status.</extracomment>
        <translation>spojení k proxy serveru odmítnuto</translation>
    </message>
    <message>
        <source>temporary failure</source>
        <extracomment>Network status.</extracomment>
        <translation>dočasný výpadek</translation>
    </message>
    <message>
        <source>authentication failed</source>
        <extracomment>Network status.</extracomment>
        <translation>autentifikace selhala</translation>
    </message>
    <message>
        <source>proxy authentication required</source>
        <extracomment>Network status.</extracomment>
        <translation>proxy autentifikace selhala</translation>
    </message>
    <message>
        <source>proxy server not found</source>
        <extracomment>Network status.</extracomment>
        <translation>proxy server nenalezen</translation>
    </message>
    <message>
        <source>uknown content</source>
        <extracomment>Network status.</extracomment>
        <translation>neznámý obsah</translation>
    </message>
    <message>
        <source>content not found</source>
        <extracomment>Network status.</extracomment>
        <translation>obsah nenalezen</translation>
    </message>
    <message>
        <source>unknown error</source>
        <extracomment>Network status.</extracomment>
        <translation>neznámá chyba</translation>
    </message>
    <message>
        <source>no errors</source>
        <extracomment>Network status.</extracomment>
        <translation>žádné chyby</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <source>LANG_NAME</source>
        <extracomment>Name of language, e.g. English.</extracomment>
        <translation>Čeština</translation>
    </message>
    <message>
        <source>LANG_ABBREV</source>
        <extracomment>Abbreviation of language, e.g. en. Use ISO 639-1 code here combined with ISO 3166-1 (alpha-2) code. Examples: &quot;cs_CZ&quot;, &quot;en_GB&quot;, &quot;en_US&quot;.</extracomment>
        <translation>cs_CZ</translation>
    </message>
    <message>
        <source>LANG_VERSION</source>
        <extracomment>Version of your translation, e.g. 1.0.</extracomment>
        <translation>0.01</translation>
    </message>
    <message>
        <source>LANG_AUTHOR</source>
        <extracomment>Name of translator - optional.</extracomment>
        <translation>Martin Rotter</translation>
    </message>
    <message>
        <source>LANG_EMAIL</source>
        <extracomment>Email of translator - optional.</extracomment>
        <translation>rotter.martinos@gmail.com</translation>
    </message>
</context>
<context>
    <name>ShortcutCatcher</name>
    <message>
        <source>Reset to original shortcut.</source>
        <translation>Obnovit původní zkratku.</translation>
    </message>
    <message>
        <source>Clear current shortcut.</source>
        <translation>Vymazat současnou zkratku.</translation>
    </message>
    <message>
        <source>Click and hit new shortcut.</source>
        <translation>Klikněte a stiskněte novou zkratku.</translation>
    </message>
</context>
<context>
    <name>StatusBar</name>
    <message>
        <source>Fullscreen mode</source>
        <translation>Mód celé obrazovky</translation>
    </message>
    <message>
        <source>Switch application between fulscreen/normal states right from this status bar icon.</source>
        <translation>Přepnout režim okna aplikace rovnou z ikonky ve stavovém pruhu.</translation>
    </message>
</context>
<context>
    <name>SystemTrayIcon</name>
    <message>
        <source>%1
Unread news: %2</source>
        <translation>%1
Nepřečtené zprávy: %2</translation>
    </message>
</context>
<context>
    <name>TabBar</name>
    <message>
        <source>Close this tab.</source>
        <translation>Zavřít tento panel.</translation>
    </message>
    <message>
        <source>Close tab</source>
        <translation>Zavřít panel</translation>
    </message>
</context>
<context>
    <name>TabWidget</name>
    <message>
        <source>Feeds</source>
        <translation>Kanály</translation>
    </message>
    <message>
        <source>Browse your feeds and messages</source>
        <translation>Procházet kanály a zprávy</translation>
    </message>
    <message>
        <source>Web browser</source>
        <extracomment>Web browser default tab title.</extracomment>
        <translation>Webový prohlížeč</translation>
    </message>
    <message>
        <source>Displays main menu.</source>
        <translation>ZObrazí hlavní menu.</translation>
    </message>
    <message>
        <source>Main menu</source>
        <translation>Hlavní menu</translation>
    </message>
    <message>
        <source>Open new web browser tab.</source>
        <translation>Otevřít nový tab webového prohlížeče.</translation>
    </message>
</context>
<context>
    <name>ToolBarEditor</name>
    <message>
        <source>Activated actions</source>
        <translation>Aktivované akce</translation>
    </message>
    <message>
        <source>Available actions</source>
        <translation>Dostupné akce</translation>
    </message>
    <message>
        <source>Insert separator</source>
        <translation>Vložit oddělovač</translation>
    </message>
    <message>
        <source>Insert spacer</source>
        <translation>Vložit mezeru</translation>
    </message>
    <message>
        <source>Separator</source>
        <translation>Oddělovač</translation>
    </message>
    <message>
        <source>Toolbar spacer</source>
        <translation>Mezera</translation>
    </message>
</context>
<context>
    <name>TrayIconMenu</name>
    <message>
        <source>Close opened modal dialogs first.</source>
        <translation>Nejdříve ukončete otevřené modální dialogy.</translation>
    </message>
</context>
<context>
    <name>WebBrowser</name>
    <message>
        <source>Navigation panel</source>
        <translation>Navigační panel</translation>
    </message>
    <message>
        <source>Back</source>
        <translation>Zpět</translation>
    </message>
    <message>
        <source>Forward</source>
        <translation>Vpřed</translation>
    </message>
    <message>
        <source>Reload</source>
        <translation>Obnovit</translation>
    </message>
    <message>
        <source>Stop</source>
        <translation>Zastavit</translation>
    </message>
    <message>
        <source>Zoom  </source>
        <translation></translation>
    </message>
    <message>
        <source>No title</source>
        <extracomment>Webbrowser tab title when no title is available.</extracomment>
        <translation>Bez názvu</translation>
    </message>
    <message>
        <source>Decrease zoom.</source>
        <translation>Oddálit aktivní webovou stránku.</translation>
    </message>
    <message>
        <source>Reset zoom to default.</source>
        <translation>Obnovit zoom na 100%.</translation>
    </message>
    <message>
        <source>Increase zoom.</source>
        <translation>Přiblížit aktivní webovou stránku.</translation>
    </message>
    <message>
        <source>Written by </source>
        <translation>Napsal </translation>
    </message>
    <message>
        <source>uknown author</source>
        <translation>neznámý autor</translation>
    </message>
    <message>
        <source>Newspaper view</source>
        <translation>Novinový náhled</translation>
    </message>
    <message>
        <source>Go back.</source>
        <translation>Jít zpět.</translation>
    </message>
    <message>
        <source>Go forward.</source>
        <translation>Jít vpřed.</translation>
    </message>
    <message>
        <source>Reload current web page.</source>
        <translation>Opět načíst aktuální webovou stránku.</translation>
    </message>
    <message>
        <source>Stop web page loading.</source>
        <translation>Zastavit načítání aktuální webové stránky.</translation>
    </message>
</context>
<context>
    <name>WebView</name>
    <message>
        <source>Reload web page</source>
        <translation>Obnovit stránku</translation>
    </message>
    <message>
        <source>Copy link url</source>
        <translation>Kopírovat adresu odkazu</translation>
    </message>
    <message>
        <source>Copy image</source>
        <translation>Kopírovat obrázek</translation>
    </message>
    <message>
        <source>Copy image url</source>
        <translation>Kopírovat adresu obrázku</translation>
    </message>
    <message>
        <source>Open link in new tab</source>
        <translation>Otevřít odkaz v novém panelu</translation>
    </message>
    <message>
        <source>Follow link</source>
        <translation>Přejít</translation>
    </message>
    <message>
        <source>Open image in new tab</source>
        <translation>Otevřít obrázek v novém panelu</translation>
    </message>
    <message>
        <source>Page not found</source>
        <translation>Stránka nenalezena</translation>
    </message>
    <message>
        <source>Check your internet connection or website address</source>
        <translation>Zkontrolujte Vaše internetové připojení a adresu webové stránky</translation>
    </message>
    <message>
        <source>This failure can be caused by:&lt;br&gt;&lt;ul&gt;&lt;li&gt;non-functional internet connection,&lt;/li&gt;&lt;li&gt;incorrect website address,&lt;/li&gt;&lt;li&gt;bad proxy server settings,&lt;/li&gt;&lt;li&gt;target destination outage,&lt;/li&gt;&lt;li&gt;many other things.&lt;/li&gt;&lt;/ul&gt;</source>
        <translation>Tuto chybu může způsobit:&lt;br&gt;&lt;ul&gt;&lt;li&gt;nefunkční internetové připojení,&lt;/li&gt;&lt;li&gt;nesprávně zadaná webová adresa,&lt;/li&gt;&lt;li&gt;špatně nastavená proxy,&lt;/li&gt;&lt;li&gt;výpadek cílového webu,&lt;/li&gt;&lt;li&gt;mnoho dalších věcí.&lt;/li&gt;&lt;/ul&gt;</translation>
    </message>
    <message>
        <source>Web browser</source>
        <translation>Webový prohlížeč</translation>
    </message>
    <message>
        <source>Image</source>
        <translation>Obrázek</translation>
    </message>
    <message>
        <source>Hyperlink</source>
        <translation>Hypertextový odkaz</translation>
    </message>
    <message>
        <source>Error page</source>
        <translation>Chyba</translation>
    </message>
    <message>
        <source>Reload current web page.</source>
        <translation>Opět načíst aktuální webovou stránku.</translation>
    </message>
    <message>
        <source>Copy selection</source>
        <translation>Kopírovat výběr</translation>
    </message>
    <message>
        <source>Copies current selection into the clipboard.</source>
        <translation>Zkopíruje aktuální výběr do schránky.</translation>
    </message>
    <message>
        <source>Copy link url to clipboard.</source>
        <translation>Kopírovat adresu odkazu do schránky.</translation>
    </message>
    <message>
        <source>Copy image to clipboard.</source>
        <translation>Kopírovat obrázek do schránky.</translation>
    </message>
    <message>
        <source>Copy image url to clipboard.</source>
        <translation>Kopírovat adresu obrázku do schránky.</translation>
    </message>
    <message>
        <source>Open this hyperlink in new tab.</source>
        <translation>Otevřít tento odkaz v novém tabu.</translation>
    </message>
    <message>
        <source>Open the hyperlink in this tab.</source>
        <translation>Otevřít tento odkaz v aktuálním tabu.</translation>
    </message>
    <message>
        <source>Open this image in this tab.</source>
        <translation>Otevřít tento obrázek v aktuálním tabu.</translation>
    </message>
    <message>
        <source>Open link in external browser</source>
        <translation>Otevřít odkaz v externím prohlížeči</translation>
    </message>
    <message>
        <source>Open the hyperlink in external browser.</source>
        <translation>Otevřít hypertextový odkaz v externím prohlížeči.</translation>
    </message>
</context>
</TS>
