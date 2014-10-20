<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="sv_SE">
<context>
    <name>Application</name>
    <message>
        <source>Application is already running.</source>
        <translation>Programmet är redan startat.</translation>
    </message>
</context>
<context>
    <name>DatabaseFactory</name>
    <message>
        <source>MySQL server works as expected.</source>
        <translation>MySQL-servern fungerar som förväntat.</translation>
    </message>
    <message>
        <source>No MySQL server is running in the target destination.</source>
        <translation>Ingen MySQL-server. körs på målplatsen.</translation>
    </message>
    <message>
        <source>Access denied. Invalid username or password used.</source>
        <extracomment>Access to MySQL server was denied.</extracomment>
        <translation>Åtkomst nekad. Ogiltigt användarnamn eller lösenord.</translation>
    </message>
    <message>
        <source>Unknown error.</source>
        <extracomment>Unknown MySQL error arised.</extracomment>
        <translation>Okänt fel.</translation>
    </message>
</context>
<context>
    <name>FeedMessageViewer</name>
    <message>
        <source>Toolbar for messages</source>
        <translation>Verktygsfält för meddelanden</translation>
    </message>
    <message>
        <source>Feed update started</source>
        <extracomment>Text display in status bar when feed update is started.</extracomment>
        <translation>Flödesuppdatering startad</translation>
    </message>
    <message>
        <source>Updated feed &apos;%1&apos;</source>
        <extracomment>Text display in status bar when particular feed is updated.</extracomment>
        <translation>Uppdaterade flödet &apos;%1&apos;</translation>
    </message>
    <message>
        <source>Cannot defragment database</source>
        <translation>Kan inte defragmentera databasen</translation>
    </message>
    <message>
        <source>Database cannot be defragmented because feed update is ongoing.</source>
        <translation>Databasen kan inte defragmenteras eftersom flödesuppdatering pågår.</translation>
    </message>
    <message>
        <source>Database defragmented</source>
        <translation>Databasen defragmenterad</translation>
    </message>
    <message>
        <source>Database was successfully defragmented.</source>
        <translation>Databasen defragmenterades korrekt.</translation>
    </message>
    <message>
        <source>Database was not defragmented</source>
        <translation>Databasen defragmenterades inte</translation>
    </message>
    <message>
        <source>Database was not defragmented. This database backend does not support it or it cannot be defragmented now.</source>
        <translation>Databasen defragmenterades inte. Stöddatabasen stöder inte defragmentering, eller så kan den inte defragmenteras just nu.</translation>
    </message>
    <message>
        <source>Toolbar for feeds</source>
        <translation>Verktygsfält för flöden</translation>
    </message>
</context>
<context>
    <name>FeedsImportExportModel</name>
    <message>
        <source> (category)</source>
        <translation> (kategori)</translation>
    </message>
    <message>
        <source> (feed)</source>
        <translation> (flöde)</translation>
    </message>
</context>
<context>
    <name>FeedsModel</name>
    <message>
        <source>Title</source>
        <extracomment>Title text in the feed list header.</extracomment>
        <translation>Namn</translation>
    </message>
    <message>
        <source>Titles of feeds/categories.</source>
        <extracomment>Feed list header &quot;titles&quot; column tooltip.</extracomment>
        <translation>Namn på kategorier/flöden.</translation>
    </message>
    <message>
        <source>Counts of unread/all meesages.</source>
        <extracomment>Feed list header &quot;counts&quot; column tooltip.</extracomment>
        <translation>Antal meddelanden.</translation>
    </message>
    <message>
        <source>Root</source>
        <extracomment>Name of root item of feed list which can be seen in feed add/edit dialog.</extracomment>
        <translation>Root</translation>
    </message>
    <message>
        <source>Invalid tree data.</source>
        <translation>Ogiltig träddata.</translation>
    </message>
    <message>
        <source>Import successfull, but some feeds/categories were not imported due to error.</source>
        <translation>Importen slutfördes, men vissa flöden/kategorier importerades inte på grund av något fel.</translation>
    </message>
    <message>
        <source>Import was completely successfull.</source>
        <translation>Importen slutfördes korrekt.</translation>
    </message>
</context>
<context>
    <name>FeedsModelCategory</name>
    <message numerus="yes">
        <source>%n unread message(s).</source>
        <extracomment>Tooltip for &quot;unread&quot; column of feed list.</extracomment>
        <translation>
            <numerusform>%n oläst meddelande.</numerusform>
            <numerusform>%n olästa meddelanden.</numerusform>
        </translation>
    </message>
    <message>
        <source>%1 (category)%2%3</source>
        <extracomment>Tooltip for standard feed.</extracomment>
        <translation>%1 (kategori)%2%3</translation>
    </message>
    <message>
        <source>
This category does not contain any nested items.</source>
        <translation>
Denna kategori innehåller inga objekt.</translation>
    </message>
</context>
<context>
    <name>FeedsModelFeed</name>
    <message>
        <source>does not use auto-update</source>
        <extracomment>Describes feed auto-update status.</extracomment>
        <translation>Uppdateras inte automatiskt</translation>
    </message>
    <message>
        <source>uses global settings</source>
        <extracomment>Describes feed auto-update status.</extracomment>
        <translation>Globala inställningar</translation>
    </message>
    <message numerus="yes">
        <source>uses specific settings (%n minute(s) to next auto-update)</source>
        <extracomment>Describes feed auto-update status.</extracomment>
        <translation>
            <numerusform>Anpassade inställningar. (%n minut till nästa auto-uppdatering)</numerusform>
            <numerusform>Anpassade inställningar. (%n minuter till nästa auto-uppdatering)</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>%n unread message(s).</source>
        <extracomment>Tooltip for &quot;unread&quot; column of feed list.</extracomment>
        <translation>
            <numerusform>%n oläst meddelande.</numerusform>
            <numerusform>%n olästa meddelanden.</numerusform>
        </translation>
    </message>
    <message>
        <source>%1 (%2)%3

Network status: %6
Encoding: %4
Auto-update status: %5</source>
        <extracomment>Tooltip for feed.</extracomment>
        <translation>%1 (%2)%3

Nätverksstatus: %6
Kodning: %4
Uppdateringsstatus: %5</translation>
    </message>
</context>
<context>
    <name>FeedsModelRecycleBin</name>
    <message>
        <source>Recycle bin</source>
        <translation>Papperskorgen</translation>
    </message>
    <message>
        <source>Recycle bin contains all deleted messages from all feeds.</source>
        <translation>Papperskorgen innehåller borttagna meddelanden från samtliga flöden.</translation>
    </message>
    <message>
        <source>Recycle bin
%1</source>
        <translation>Papperskorgen
%1</translation>
    </message>
    <message numerus="yes">
        <source>%n deleted message(s).</source>
        <translation>
            <numerusform>%n borttaget meddelande.</numerusform>
            <numerusform>%n borttagna meddelanden.</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>FeedsToolBar</name>
    <message>
        <source>Toolbar spacer</source>
        <translation>Avgränsare för verktygsfält</translation>
    </message>
</context>
<context>
    <name>FeedsView</name>
    <message>
        <source>Context menu for feeds</source>
        <translation>Kontextmeny för flöden</translation>
    </message>
    <message>
        <source>Cannot add standard category</source>
        <translation>Kan inte lägga till kategori</translation>
    </message>
    <message>
        <source>You cannot add new standard category now because feed update is ongoing.</source>
        <translation>Du kan inte lägga till ny kategori nu, eftersom flödesuppdatering pågår.</translation>
    </message>
    <message>
        <source>Cannot add standard feed</source>
        <translation>Kan inte lägga till flöde</translation>
    </message>
    <message>
        <source>You cannot add new standard feed now because feed update is ongoing.</source>
        <translation>Du kan inte lägga till nytt flöde nu, eftersom flödesuppdatering pågår.</translation>
    </message>
    <message>
        <source>Cannot edit item</source>
        <translation>Kan inte redigera objektet</translation>
    </message>
    <message>
        <source>Selected item cannot be edited because feed update is ongoing.</source>
        <translation>Det markerade objektet kan inte redigeras eftersom flödesuppdatering pågår.</translation>
    </message>
    <message>
        <source>Cannot delete item</source>
        <translation>Kan inte bort objektet</translation>
    </message>
    <message>
        <source>Selected item cannot be deleted because feed update is ongoing.</source>
        <translation>Markerat objekt kan inte tas bort eftersom flödesuppdatering pågår.</translation>
    </message>
    <message>
        <source>Cannot update all items</source>
        <translation>Kan inte uppdatera alla objekt</translation>
    </message>
    <message>
        <source>You cannot update all items because another feed update is ongoing.</source>
        <translation>Du kan inte uppdatera alla objekt eftersom flödesuppdatering redan pågår.</translation>
    </message>
    <message>
        <source>Cannot update selected items</source>
        <translation>Kan inte uppdatera markerat objekt</translation>
    </message>
    <message>
        <source>You cannot update selected items because another feed update is ongoing.</source>
        <translation>Du kan inte uppdatera markerat objekt eftersom flödesuppdatering redan pågår.</translation>
    </message>
    <message>
        <source>You are about to delete selected feed or category.</source>
        <translation>Du är på väg att ta bort markerat flöde eller kategori.</translation>
    </message>
    <message>
        <source>Deletion of item failed.</source>
        <translation>Borttagningen misslyckades.</translation>
    </message>
    <message>
        <source>Selected item was not deleted due to error.</source>
        <translation>Objektet togs inte bort, på grund av ett fel.</translation>
    </message>
    <message>
        <source>Deleting feed or category</source>
        <translation>Tar bort flöde/kategori</translation>
    </message>
    <message>
        <source>Do you really want to delete selected item?</source>
        <translation>Vill du verkligen ta bort markerat objekt?</translation>
    </message>
    <message>
        <source>Permanently delete messages</source>
        <translation>Ta bort meddelanden permanent</translation>
    </message>
    <message>
        <source>You are about to permanenty delete all messages from your recycle bin.</source>
        <translation>Du är på väg att permanent ta bort alla meddelanden från papperskorgen.</translation>
    </message>
    <message>
        <source>Do you really want to empty your recycle bin?</source>
        <translation>Vill du verkligen tömma papperskorgen?</translation>
    </message>
    <message>
        <source>Context menu for empty space</source>
        <translation>Kontextmeny för tomt utrymme</translation>
    </message>
    <message>
        <source>Context menu for recycle bin</source>
        <translation>Kontextmeny för papperskorgen</translation>
    </message>
</context>
<context>
    <name>FormAbout</name>
    <message>
        <source>Information</source>
        <translation>Information</translation>
    </message>
    <message>
        <source>Licenses</source>
        <translation>Licenser</translation>
    </message>
    <message>
        <source>GNU GPL License (applies to RSS Guard source code)</source>
        <translation>GNU GPL Licens (gäller RSS Guards källkod)</translation>
    </message>
    <message>
        <source>GNU GPL License</source>
        <translation>GNU GPL Licens</translation>
    </message>
    <message>
        <source>BSD License (applies to QtSingleApplication source code)</source>
        <translation>BSD Licens (gäller to QtSingleApplication källkod)</translation>
    </message>
    <message>
        <source>Licenses page is available only in English language.</source>
        <translation>Licenssidan visas endast på Engelska.</translation>
    </message>
    <message>
        <source>Changelog</source>
        <translation>Ändringslogg</translation>
    </message>
    <message>
        <source>Changelog page is available only in English language.</source>
        <translation>Ändringsloggen visas endast på Engelska.</translation>
    </message>
    <message>
        <source>License not found.</source>
        <translation>Licensen hittades inte.</translation>
    </message>
    <message>
        <source>Changelog not found.</source>
        <translation>Ändringsloggen hittades inte.</translation>
    </message>
    <message>
        <source>&lt;b&gt;%8&lt;/b&gt;&lt;br&gt;&lt;b&gt;Version:&lt;/b&gt; %1 (build on %2 with CMake %3)&lt;br&gt;&lt;b&gt;Revision:&lt;/b&gt; %4&lt;br&gt;&lt;b&gt;Build date:&lt;/b&gt; %5&lt;br&gt;&lt;b&gt;Qt:&lt;/b&gt; %6 (compiled against %7)&lt;br&gt;</source>
        <translation>&lt;b&gt;%8&lt;/b&gt;&lt;br&gt;&lt;b&gt;Version:&lt;/b&gt; %1 (byggd på %2 med CMake %3)&lt;br&gt;&lt;b&gt;Revision:&lt;/b&gt; %4&lt;br&gt;&lt;b&gt;Byggdatum:&lt;/b&gt; %5&lt;br&gt;&lt;b&gt;Qt:&lt;/b&gt; %6 (kompilerad mot %7)&lt;br&gt;</translation>
    </message>
    <message>
        <source>&lt;body&gt;%5 is a (very) tiny feed reader.&lt;br&gt;&lt;br&gt;This software is distributed under the terms of GNU General Public License, version 3.&lt;br&gt;&lt;br&gt;Contacts:&lt;ul&gt;&lt;li&gt;&lt;a href=&quot;mailto://%1&quot;&gt;%1&lt;/a&gt; ~email&lt;/li&gt;&lt;li&gt;&lt;a href=&quot;%2&quot;&gt;%2&lt;/a&gt; ~website&lt;/li&gt;&lt;/ul&gt;You can obtain source code for %5 from its website.&lt;br&gt;&lt;br&gt;&lt;br&gt;Copyright (C) 2011-%3 %4&lt;/body&gt;</source>
        <translation>&lt;body&gt;%5 är en (mycket) lätt flödesläsare.&lt;br&gt;&lt;br&gt;Mjukvaran distribueras under villkoren för GNU General Public Licens, version 3.&lt;br&gt;&lt;br&gt;Kontakt:&lt;ul&gt;&lt;li&gt;&lt;a href=&quot;mailto://%1&quot;&gt;%1&lt;/a&gt; ~e-post&lt;/li&gt;&lt;li&gt;&lt;a href=&quot;%2&quot;&gt;%2&lt;/a&gt; ~webbsida&lt;/li&gt;&lt;/ul&gt;Du kan hämmta källkoden för %5 från webbsidan.&lt;br&gt;&lt;br&gt;&lt;br&gt;Copyright (C) 2011-%3 %4&lt;/body&gt;</translation>
    </message>
    <message>
        <source>About %1</source>
        <extracomment>About RSS Guard dialog title.</extracomment>
        <translation>Om %1</translation>
    </message>
    <message>
        <source>Paths</source>
        <translation>Sökvägar</translation>
    </message>
    <message>
        <source>Settings type</source>
        <translation>Inställningstyp</translation>
    </message>
    <message>
        <source>Settings file</source>
        <translation>Inställningsfil</translation>
    </message>
    <message>
        <source>Database root path</source>
        <translation>Databasens root-mapp</translation>
    </message>
    <message>
        <source>FULLY portable</source>
        <translation>FULLT portabel</translation>
    </message>
    <message>
        <source>PARTIALLY portable</source>
        <translation>PARTIELLT portabel</translation>
    </message>
</context>
<context>
    <name>FormBackupDatabaseSettings</name>
    <message>
        <source>Backup database/settings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&amp;Select folder</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Backup properties</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Items to backup</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Database</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Settings</source>
        <translation type="unfinished">Inställningar</translation>
    </message>
    <message>
        <source>Backup name</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Operation results</source>
        <translation type="unfinished">Åtgärdsresultat</translation>
    </message>
    <message>
        <source>Common name for backup files</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No operation executed yet.</source>
        <translation type="unfinished">Ingen åtgärd slutförd än.</translation>
    </message>
    <message>
        <source>Backup was created successfully and stored in target folder.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Backup was created successfully.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Backup failed, database and/or settings is probably not backed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Backup failed. Check the output folder if your database
and/or settings were backed or not. Also make sure that target foder is writable.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Select destionation folder</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Good destination folder is specified.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Backup name cannot be empty.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Backup name looks okay.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>FormCategoryDetails</name>
    <message>
        <source>Parent category</source>
        <translation>Överordnad kategori</translation>
    </message>
    <message>
        <source>Select parent item for your category.</source>
        <translation>Välj överordnad mapp för kategorin.</translation>
    </message>
    <message>
        <source>Title</source>
        <translation>Namn</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Beskrivning</translation>
    </message>
    <message>
        <source>Icon</source>
        <translation>Ikon</translation>
    </message>
    <message>
        <source>Select icon for your category.</source>
        <translation>Välj ikon för kategorin.</translation>
    </message>
    <message>
        <source>Add new category</source>
        <translation>Lägg till ny kategori</translation>
    </message>
    <message>
        <source>Edit existing category</source>
        <translation>Redigera befintlig kategori</translation>
    </message>
    <message>
        <source>Cannot add category</source>
        <translation>Kan inte lägga till kategori</translation>
    </message>
    <message>
        <source>Category was not added due to error.</source>
        <translation>Kategorin lades inte till, på grund av något fel.</translation>
    </message>
    <message>
        <source>Cannot edit category</source>
        <translation>Kan inte redigera kategorin</translation>
    </message>
    <message>
        <source>Category was not edited due to error.</source>
        <translation>Kategorin kan inte redigeras, på grund av något fel.</translation>
    </message>
    <message>
        <source>Category name is ok.</source>
        <translation>Kategorinamnet är ok.</translation>
    </message>
    <message>
        <source>Category name is too short.</source>
        <translation>Kategorinamnet är för kort.</translation>
    </message>
    <message>
        <source>Description is empty.</source>
        <translation>Beskrivning saknas.</translation>
    </message>
    <message>
        <source>Select icon file for the category</source>
        <translation>Välj ikonfil för kategorin</translation>
    </message>
    <message>
        <source>Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)</source>
        <translation>Bilder (*.bmp *.jpg *.jpeg *.png *.svg *.tga)</translation>
    </message>
    <message>
        <source>Select icon</source>
        <translation>Välj ikon</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Avbryt</translation>
    </message>
    <message>
        <source>Look in:</source>
        <extracomment>Label to describe the folder for icon file selection dialog.</extracomment>
        <translation>Sök i:</translation>
    </message>
    <message>
        <source>Icon name:</source>
        <translation>Ikonnamn:</translation>
    </message>
    <message>
        <source>Icon type:</source>
        <translation>Ikontyp:</translation>
    </message>
    <message>
        <source>Category title</source>
        <translation>Kategorinamn</translation>
    </message>
    <message>
        <source>Set title for your category.</source>
        <translation>Ange namnet på din kategori.</translation>
    </message>
    <message>
        <source>Category description</source>
        <translation>Kategoribeskrivning</translation>
    </message>
    <message>
        <source>Set description for your category.</source>
        <translation>Beskriv din kategori.</translation>
    </message>
    <message>
        <source>Icon selection</source>
        <translation>Ikonval</translation>
    </message>
    <message>
        <source>Load icon from file...</source>
        <translation>Hämta ikon från fil...</translation>
    </message>
    <message>
        <source>Do not use icon</source>
        <translation>Använd ingen ikon</translation>
    </message>
    <message>
        <source>Use default icon</source>
        <translation>Använd standardikon</translation>
    </message>
    <message>
        <source>The description is ok.</source>
        <translation>Beskrivningen är ok.</translation>
    </message>
</context>
<context>
    <name>FormFeedDetails</name>
    <message>
        <source>Parent category</source>
        <translation>Överordnad kategori</translation>
    </message>
    <message>
        <source>Select parent item for your feed.</source>
        <translation>Välj överordnad mapp för flödet.</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>Typ</translation>
    </message>
    <message>
        <source>Select type of the standard feed.</source>
        <translation>Välj flödestyp.</translation>
    </message>
    <message>
        <source>Encoding</source>
        <translation>Kodning</translation>
    </message>
    <message>
        <source>Select encoding of the standard feed. If you are unsure about the encoding, then select &quot;UTF-8&quot; encoding.</source>
        <translation>Välj flödeskodning. Välj &quot;UTF-8&quot; om du är osäker på kodningen.</translation>
    </message>
    <message>
        <source>Auto-update</source>
        <translation>Auto-uppdatering</translation>
    </message>
    <message>
        <source>Select the auto-update strategy for this feed. Default auto-update strategy means that the feed will be update in time intervals set in application settings.</source>
        <translation>Välj uppdateringsstrategi för flödet. Global auto-uppdatering, innebär att flödet kommer att uppdateras med tidsintervall angivna i programinställningarna.</translation>
    </message>
    <message>
        <source> minutes</source>
        <translation>minuter</translation>
    </message>
    <message>
        <source>Title</source>
        <translation>Namn</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Beskrivning</translation>
    </message>
    <message>
        <source>URL</source>
        <translation>URL</translation>
    </message>
    <message>
        <source>Fetch it now</source>
        <translation>Hämta nu</translation>
    </message>
    <message>
        <source>Icon</source>
        <translation>Ikon</translation>
    </message>
    <message>
        <source>Select icon for your feed.</source>
        <translation>Välj ikon för flödet.</translation>
    </message>
    <message>
        <source>Some feeds require authentication, including GMail feeds. BASIC, NTLM-2 and DIGEST-MD5 authentication schemes are supported.</source>
        <translation>Vissa flöden kräver autentisering, inklusive Gmail-flöden. BASIC, NTLM-2 och DIGEST-MD5 autentisering stöds.</translation>
    </message>
    <message>
        <source>Requires authentication</source>
        <translation>Kräver autentisering</translation>
    </message>
    <message>
        <source>Username</source>
        <translation>Användarnamn</translation>
    </message>
    <message>
        <source>Password</source>
        <translation>Lösenord</translation>
    </message>
    <message>
        <source>Fetch metadata</source>
        <translation>Hämta metadata</translation>
    </message>
    <message>
        <source>Add new feed</source>
        <translation>Lägg till nytt flöde</translation>
    </message>
    <message>
        <source>Edit existing feed</source>
        <translation>Redigera befintligt flöde</translation>
    </message>
    <message>
        <source>Feed name is ok.</source>
        <translation>Flödesnamnet är ok.</translation>
    </message>
    <message>
        <source>Feed name is too short.</source>
        <translation>Flödesnamnet är för kort.</translation>
    </message>
    <message>
        <source>Description is empty.</source>
        <translation>Beskrivning saknas.</translation>
    </message>
    <message>
        <source>The url is ok.</source>
        <translation>Webbadressen är ok.</translation>
    </message>
    <message>
        <source>The url does not meet standard pattern. Does your url start with &quot;http://&quot; or &quot;https://&quot; prefix.</source>
        <translation>Webbadressen liknar inte standardmönstret. Börjar din URL med prefixet &quot;http://&quot; eller &quot;https://&quot;?.</translation>
    </message>
    <message>
        <source>The url is empty.</source>
        <translation>URL saknas.</translation>
    </message>
    <message>
        <source>Username is ok or it is not needed.</source>
        <translation>Användarnamnet är ok, eller behövs inte.</translation>
    </message>
    <message>
        <source>Username is empty.</source>
        <translation>Användarnamn saknas.</translation>
    </message>
    <message>
        <source>Password is ok or it is not needed.</source>
        <translation>Lösenordet är ok, eller behövs inte.</translation>
    </message>
    <message>
        <source>Password is empty.</source>
        <translation>Lösenord saknas.</translation>
    </message>
    <message>
        <source>Select icon file for the feed</source>
        <translation>Välj ikonfil för flödet</translation>
    </message>
    <message>
        <source>Images (*.bmp *.jpg *.jpeg *.png *.svg *.tga)</source>
        <translation>bilder (*.bmp *.jpg *.jpeg *.png *.svg *.tga)</translation>
    </message>
    <message>
        <source>Select icon</source>
        <translation>Välj ikon</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Avbryt</translation>
    </message>
    <message>
        <source>Look in:</source>
        <extracomment>Label for field with icon file name textbox for selection dialog.</extracomment>
        <translation>Sök i:</translation>
    </message>
    <message>
        <source>Icon name:</source>
        <translation>Ikonnamn:</translation>
    </message>
    <message>
        <source>Icon type:</source>
        <translation>Ikontyp:</translation>
    </message>
    <message>
        <source>Cannot add feed</source>
        <translation>Kan inte lägga till flöde</translation>
    </message>
    <message>
        <source>Feed was not added due to error.</source>
        <translation>Flödet lades inte till, på grund av något fel.</translation>
    </message>
    <message>
        <source>Cannot edit feed</source>
        <translation>Kan inte redigera flödet</translation>
    </message>
    <message>
        <source>All metadata fetched successfully.</source>
        <translation>All metadata hämtades korrekt.</translation>
    </message>
    <message>
        <source>Feed and icon metadata fetched.</source>
        <translation>Flödes- och ikonmetadata hämtad.</translation>
    </message>
    <message>
        <source>Result: %1.</source>
        <translation>Resultat: %1.</translation>
    </message>
    <message>
        <source>Feed or icon metatada not fetched.</source>
        <translation>Flödes- eller ikonmetadata hämtades inte.</translation>
    </message>
    <message>
        <source>Error: %1.</source>
        <translation>Fel: %1.</translation>
    </message>
    <message>
        <source>No metadata fetched.</source>
        <translation>Ingen metadata hämtades.</translation>
    </message>
    <message>
        <source>Feed title</source>
        <translation>Flödesnamn</translation>
    </message>
    <message>
        <source>Set title for your feed.</source>
        <translation>Ange flödets namn.</translation>
    </message>
    <message>
        <source>Feed description</source>
        <translation>Flödesbeskrivning</translation>
    </message>
    <message>
        <source>Set description for your feed.</source>
        <translation>Beskriv flödet.</translation>
    </message>
    <message>
        <source>Full feed url including scheme</source>
        <translation>Flödets fullständiga webbadress (URL)</translation>
    </message>
    <message>
        <source>Set url for your feed.</source>
        <translation>Ange flödets URL.</translation>
    </message>
    <message>
        <source>Set username to access the feed.</source>
        <translation>Ange användarnamn för att få åtkomst till flödet.</translation>
    </message>
    <message>
        <source>Set password to access the feed.</source>
        <translation>Ange lösenord för att få åtkomst till flödet.</translation>
    </message>
    <message>
        <source>Icon selection</source>
        <translation>Ikonval</translation>
    </message>
    <message>
        <source>Load icon from file...</source>
        <translation>Hämta ikon från fil...</translation>
    </message>
    <message>
        <source>Do not use icon</source>
        <translation>Använd ingen ikon</translation>
    </message>
    <message>
        <source>Use default icon</source>
        <translation>Använd standardikon</translation>
    </message>
    <message>
        <source>No metadata fetched so far.</source>
        <translation>Ingen metadata hämtad.</translation>
    </message>
    <message>
        <source>Auto-update using global interval</source>
        <translation>Global auto-uppdatering</translation>
    </message>
    <message>
        <source>Auto-update every</source>
        <translation>Auto-uppdatera varje</translation>
    </message>
    <message>
        <source>Do not auto-update at all</source>
        <translation>Ingen auto-uppdatering</translation>
    </message>
    <message>
        <source>The description is ok.</source>
        <translation>Beskrivningen är ok.</translation>
    </message>
    <message>
        <source>Feed was not edited due to error.</source>
        <translation>Flödet redigerades inte, på grund av något fel.</translation>
    </message>
</context>
<context>
    <name>FormImportExport</name>
    <message>
        <source>&amp;Select file</source>
        <translation>&amp;Välj fil</translation>
    </message>
    <message>
        <source>Operation results</source>
        <translation>Åtgärdsresultat</translation>
    </message>
    <message>
        <source>No file is selected.</source>
        <translation>Ingen fil har valts.</translation>
    </message>
    <message>
        <source>No operation executed yet.</source>
        <translation>Ingen åtgärd slutförd än.</translation>
    </message>
    <message>
        <source>Export feeds</source>
        <translation>Exportera flöden</translation>
    </message>
    <message>
        <source>Destination file</source>
        <translation>Målfil</translation>
    </message>
    <message>
        <source>Source feeds &amp;&amp; categories</source>
        <translation>Källflöden &amp;&amp; -kategorier</translation>
    </message>
    <message>
        <source>Source file</source>
        <translation>Källfil</translation>
    </message>
    <message>
        <source>Target feeds &amp;&amp; categories</source>
        <translation>Målflöden &amp;&amp; -kategorier</translation>
    </message>
    <message>
        <source>Import feeds</source>
        <translation>Importera flöden</translation>
    </message>
    <message>
        <source>OPML 2.0 files (*.opml)</source>
        <translation>OPML 2.0-filer (*.opml)</translation>
    </message>
    <message>
        <source>Select file for feeds export</source>
        <translation>Välj fil för flödesexport</translation>
    </message>
    <message>
        <source>File is selected.</source>
        <translation>Fil är vald.</translation>
    </message>
    <message>
        <source>Select file for feeds import</source>
        <translation>Välj fil för flödesimport</translation>
    </message>
    <message>
        <source>Cannot open source file.</source>
        <translation>Kan inte öppna källfil.</translation>
    </message>
    <message>
        <source>Feeds were loaded.</source>
        <translation>Flöden lästes in.</translation>
    </message>
    <message>
        <source>Error, file is not well-formed. Select another file.</source>
        <translation>Fel! Filen är inte rätt formaterad. Välj en annan fil.</translation>
    </message>
    <message>
        <source>Error occurred. File is not well-formed. Select another file.</source>
        <translation>Ett fel uppstod. Filen är felformaterad. Välj en annan fil.</translation>
    </message>
    <message>
        <source>Feeds were exported successfully.</source>
        <translation>Flöden exporterades korrekt.</translation>
    </message>
    <message>
        <source>Cannot write into destination file.</source>
        <translation>Kan inte skriva till målfilen.</translation>
    </message>
    <message>
        <source>Critical error occurred.</source>
        <translation>Ett allvarligt fel uppstod.</translation>
    </message>
    <message>
        <source>&amp;Check all items</source>
        <translation>&amp;Markera alla</translation>
    </message>
    <message>
        <source>&amp;Uncheck all items</source>
        <translation>&amp;Avmarkera alla</translation>
    </message>
</context>
<context>
    <name>FormMain</name>
    <message>
        <source>&amp;File</source>
        <translation>&amp;Arkiv</translation>
    </message>
    <message>
        <source>&amp;Help</source>
        <translation>&amp;Hjälp</translation>
    </message>
    <message>
        <source>&amp;View</source>
        <translation>V&amp;isa</translation>
    </message>
    <message>
        <source>&amp;Tools</source>
        <translation>V&amp;erktyg</translation>
    </message>
    <message>
        <source>&amp;Quit</source>
        <translation>A&amp;vsluta</translation>
    </message>
    <message>
        <source>&amp;Settings</source>
        <translation>&amp;Inställningar</translation>
    </message>
    <message>
        <source>&amp;Current tab</source>
        <translation>Aktuell &amp;flik</translation>
    </message>
    <message>
        <source>&amp;Add tab</source>
        <translation>&amp;Lägg till flik</translation>
    </message>
    <message>
        <source>&amp;Messages</source>
        <translation>&amp;Meddelanden</translation>
    </message>
    <message>
        <source>&amp;Web browser</source>
        <translation>&amp;Webbläsare</translation>
    </message>
    <message>
        <source>Switch &amp;importance of selected messages</source>
        <translation>Växla &amp;prioritet för markerade meddelanden</translation>
    </message>
    <message>
        <source>Quit the application.</source>
        <translation>Avsluta programmet.</translation>
    </message>
    <message>
        <source>Display settings of the application.</source>
        <translation>Visa inställningar för programmet.</translation>
    </message>
    <message>
        <source>Switch fullscreen mode.</source>
        <translation>Växla fönster-/fullskärmsläge.</translation>
    </message>
    <message>
        <source>Add new web browser tab.</source>
        <translation>Lägg till ny webbläsarflik.</translation>
    </message>
    <message>
        <source>Close current web browser tab.</source>
        <translation>Stäng aktuell webbläsarflik.</translation>
    </message>
    <message>
        <source>No actions available</source>
        <translation>Inga åtgärder tillgängliga</translation>
    </message>
    <message>
        <source>No actions are available right now.</source>
        <translation>Inga åtgärder tillgängliga just nu.</translation>
    </message>
    <message>
        <source>Fee&amp;ds &amp;&amp; categories</source>
        <translation>&amp;Flöden &amp;&amp; kategorier</translation>
    </message>
    <message>
        <source>Mark all messages (without message filters) from selected feeds as read.</source>
        <translation>Markera alla meddelanden från valda flöden, som lästa.</translation>
    </message>
    <message>
        <source>Mark all messages (without message filters) from selected feeds as unread.</source>
        <translation>Markera alla meddelanden från valda flöden, som olästa.</translation>
    </message>
    <message>
        <source>Displays all messages from selected feeds/categories in a new &quot;newspaper mode&quot; tab. Note that messages are not set as read automatically.</source>
        <translation>Visa alla meddelanden från markerade flöden/kategorier i en ny flik, som &quot;tidningsvy&quot;. Notera att meddelandena inte automatiskt markeras som lästa.</translation>
    </message>
    <message>
        <source>Hides main window if it is visible and shows it if it is hidden.</source>
        <translation>Dölj programfönstret om det är synligt, och visa det om det är dolt.</translation>
    </message>
    <message>
        <source>Defragment database file so that its size decreases.</source>
        <translation>Defragmentera databasfilen för att minska storleken.</translation>
    </message>
    <message>
        <source>Hides or shows the list of feeds/categories.</source>
        <translation>Dölj/Visa listan med flöden/kategorier.</translation>
    </message>
    <message>
        <source>Check if new update for the application is available for download.</source>
        <translation>Sök efter ny programuppdatering.</translation>
    </message>
    <message>
        <source>Cannot check for updates</source>
        <translation>Kan inte söka efter uppdateringar</translation>
    </message>
    <message>
        <source>You cannot check for updates because feed update is ongoing.</source>
        <translation>Du kan inte söka efter uppdateringar, på grund av pågående flödesuppdatering.</translation>
    </message>
    <message>
        <source>&amp;About application</source>
        <translation>&amp;Om programmet</translation>
    </message>
    <message>
        <source>Displays extra info about this application.</source>
        <translation>Visa extra information om det här programmet.</translation>
    </message>
    <message>
        <source>&amp;Delete selected messages</source>
        <translation>&amp;Ta bort markerade meddelanden</translation>
    </message>
    <message>
        <source>Deletes all messages from selected feeds.</source>
        <translation>Ta bort alla meddelanden från markerade flöden.</translation>
    </message>
    <message>
        <source>Marks all messages in all feeds read. This does not take message filters into account.</source>
        <translation>Markera alla meddelanden i samtliga flöden som lästa. Detta åsidosätter eventuella meddelandefilter.</translation>
    </message>
    <message>
        <source>Deletes all messages from all feeds.</source>
        <translation>Ta bort alla meddelanden från samtliga flöden.</translation>
    </message>
    <message>
        <source>Update &amp;all feeds</source>
        <translation>Uppdatera &amp;alla flöden</translation>
    </message>
    <message>
        <source>Update &amp;selected feeds</source>
        <translation>Uppdatera &amp;markerade flöden</translation>
    </message>
    <message>
        <source>&amp;Edit selected feed/category</source>
        <translation>&amp;Redigera markerat flöde/kategori</translation>
    </message>
    <message>
        <source>&amp;Delete selected feed/category</source>
        <translation>&amp;Ta bort markerat flöde/kategori</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Inställningar</translation>
    </message>
    <message>
        <source>Hides or displays the main menu.</source>
        <translation>Dölj/Visa huvudmenyn.</translation>
    </message>
    <message>
        <source>Add &amp;new feed/category</source>
        <translation>Lägg till &amp;nytt flöde/kategori</translation>
    </message>
    <message>
        <source>&amp;Close all tabs except current one</source>
        <translation>&amp;Stäng alla flikar utom den aktuella</translation>
    </message>
    <message>
        <source>&amp;Close current tab</source>
        <translation>&amp;Stäng aktuell flik</translation>
    </message>
    <message>
        <source>Mark &amp;selected messages as &amp;read</source>
        <translation>Märk markerade &amp;meddelanden som &amp;lästa</translation>
    </message>
    <message>
        <source>Mark &amp;selected messages as &amp;unread</source>
        <translation>Märk markerade &amp;meddelanden som &amp;olästa</translation>
    </message>
    <message>
        <source>&amp;Mark selected feeds as read</source>
        <translation>&amp;Märk markerade meddelanden som lästa</translation>
    </message>
    <message>
        <source>&amp;Mark selected feeds as unread</source>
        <translation>&amp;Märk markerade meddelanden som olästa</translation>
    </message>
    <message>
        <source>&amp;Clean selected feeds</source>
        <translation>&amp;Rensa markerade flöden</translation>
    </message>
    <message>
        <source>Open selected source articles in &amp;external browser</source>
        <translation>Öppna markerade källartiklar i &amp;extern webbläsare</translation>
    </message>
    <message>
        <source>Open selected messages in &amp;internal browser</source>
        <translation>Öppna markerade meddelanden i &amp;intern webbläsare</translation>
    </message>
    <message>
        <source>Open selected source articles in &amp;internal browser</source>
        <translation>Öppna markerade källartiklar i &amp;intern webbläsare</translation>
    </message>
    <message>
        <source>&amp;Mark all feeds as &amp;read</source>
        <translation>&amp;Markera samtliga flöden som &amp;lästa</translation>
    </message>
    <message>
        <source>View selected feeds in &amp;newspaper mode</source>
        <translation>Visa markerade flöden som &amp;tidningsvy</translation>
    </message>
    <message>
        <source>&amp;Defragment database</source>
        <translation>&amp;Defragmentera databasen</translation>
    </message>
    <message>
        <source>&amp;Clean all feeds</source>
        <translation>&amp;Rensa alla flöden</translation>
    </message>
    <message>
        <source>Select &amp;next feed/category</source>
        <translation>Gå till &amp;nästa flöde/kategori</translation>
    </message>
    <message>
        <source>Select &amp;previous feed/category</source>
        <translation>Gå till &amp;föregående flöde/kategori</translation>
    </message>
    <message>
        <source>Select &amp;next message</source>
        <translation>Gå till &amp;nästa meddelande</translation>
    </message>
    <message>
        <source>Select &amp;previous message</source>
        <translation>Gå till &amp;föregående meddelande</translation>
    </message>
    <message>
        <source>Check for &amp;updates</source>
        <translation>Sök efter &amp;uppdateringar</translation>
    </message>
    <message>
        <source>Enable &amp;JavaScript</source>
        <translation>Aktivera &amp;JavaScript</translation>
    </message>
    <message>
        <source>Enable external &amp;plugins</source>
        <translation>Aktivera externa &amp;tillägg</translation>
    </message>
    <message>
        <source>Auto-load &amp;images</source>
        <translation>Läs in &amp;bilder automatiskt</translation>
    </message>
    <message>
        <source>Show/hide</source>
        <translation>Dölj/Visa</translation>
    </message>
    <message>
        <source>&amp;Fullscreen</source>
        <translation>&amp;Fullskärm</translation>
    </message>
    <message>
        <source>&amp;Feed list</source>
        <translation>&amp;Flödeslista</translation>
    </message>
    <message>
        <source>&amp;Main menu</source>
        <translation>&amp;Huvudmeny</translation>
    </message>
    <message>
        <source>Switch visibility of main &amp;window</source>
        <translation>Visa/Dölj &amp;programfönstret</translation>
    </message>
    <message>
        <source>Cannot open external browser</source>
        <translation>Kan inte öppna extern webbläsare</translation>
    </message>
    <message>
        <source>Cannot open external browser. Navigate to application website manually.</source>
        <translation>Kan inte öppna extern webbläsare. Navigera manuellt till programmets webbsida.</translation>
    </message>
    <message>
        <source>New &amp;feed</source>
        <translation>Nytt &amp;flöde</translation>
    </message>
    <message>
        <source>Add new feed.</source>
        <translation>Lägg till nytt flöde.</translation>
    </message>
    <message>
        <source>New &amp;category</source>
        <translation>Ny &amp;kategori</translation>
    </message>
    <message>
        <source>Add new category.</source>
        <translation>Lägg till ny kategori.</translation>
    </message>
    <message>
        <source>&amp;Toolbars</source>
        <translation>&amp;Verktygsfält</translation>
    </message>
    <message>
        <source>Switch visibility of main toolbars.</source>
        <translation>Visa/Dölj verktygsfält.</translation>
    </message>
    <message>
        <source>&amp;Feed/message list headers</source>
        <translation>&amp;Kolumnrubriker</translation>
    </message>
    <message>
        <source>&amp;Import feeds</source>
        <translation>&amp;Importera flöden</translation>
    </message>
    <message>
        <source>Imports feeds you want from selected file.</source>
        <translation>Importera flöden från fil.</translation>
    </message>
    <message>
        <source>&amp;Export feeds</source>
        <translation>&amp;Exportera flöden</translation>
    </message>
    <message>
        <source>Exports feeds you want to selected file.</source>
        <translation>Exportera flöden till fil.</translation>
    </message>
    <message>
        <source>Close all tabs except current one.</source>
        <translation>Stäng alla flikar utom aktuell.</translation>
    </message>
    <message>
        <source>&amp;Recycle bin</source>
        <translation>&amp;Papperskorgen</translation>
    </message>
    <message>
        <source>Report a &amp;bug (GitHub)...</source>
        <translation>Rapportera ett &amp;fel (GitHub)...</translation>
    </message>
    <message>
        <source>Report a bug (BitBucket)...</source>
        <translation>Rapportera ett fel (BitBucket)...</translation>
    </message>
    <message>
        <source>&amp;Donate via PayPal</source>
        <translation>&amp;Donera via PayPal</translation>
    </message>
    <message>
        <source>Display &amp;wiki</source>
        <translation>Visa &amp;wiki</translation>
    </message>
    <message>
        <source>&amp;Empty recycle bin</source>
        <translation>&amp;Töm papperskorgen</translation>
    </message>
    <message>
        <source>&amp;Restore all messages</source>
        <translation>&amp;Återställ alla meddelanden</translation>
    </message>
    <message>
        <source>Restore &amp;selected messages</source>
        <translation>Återställ &amp;markerade meddelanden</translation>
    </message>
    <message>
        <source>&amp;Restart</source>
        <translation>&amp;Starta om</translation>
    </message>
    <message>
        <source>&amp;Restore database/settings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>&amp;Backup database/settings</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>FormRestoreDatabaseSettings</name>
    <message>
        <source>Restore database/settings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Operation results</source>
        <translation type="unfinished">Åtgärdsresultat</translation>
    </message>
    <message>
        <source>&amp;Select folder</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Restore database</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Restore settings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Restart</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>No operation executed yet.</source>
        <translation type="unfinished">Ingen åtgärd slutförd än.</translation>
    </message>
    <message>
        <source>Restoration was initiated. Restart to proceed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>You need to restart application for restoration process to finish.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Restoration was not initiated successfully.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Database and/or settings were not copied to restoration folder successully.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Select source folder</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Good source folder is specified.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>FormSettings</name>
    <message>
        <source>General</source>
        <extracomment>General settings section.</extracomment>
        <translation>Allmänt</translation>
    </message>
    <message>
        <source>User interface</source>
        <translation>Utseende</translation>
    </message>
    <message>
        <source>Icon theme</source>
        <translation>Ikontema</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Inställningar</translation>
    </message>
    <message>
        <source>Keyboard shortcuts</source>
        <translation>Tangentbordsgenvägar</translation>
    </message>
    <message>
        <source>Language</source>
        <extracomment>Language settings section.
----------
Language column of language list.</extracomment>
        <translation>Språk</translation>
    </message>
    <message>
        <source>Proxy</source>
        <translation>Proxy</translation>
    </message>
    <message>
        <source>Icons &amp;&amp; skins</source>
        <translation>Ikoner &amp;&amp; teman</translation>
    </message>
    <message>
        <source>Tray icon</source>
        <translation>Meddelandefältsikon</translation>
    </message>
    <message>
        <source>Start application hidden</source>
        <translation>Starta programmet dolt</translation>
    </message>
    <message>
        <source>Type</source>
        <extracomment>Proxy server type.</extracomment>
        <translation>Typ</translation>
    </message>
    <message>
        <source>Host</source>
        <translation>Värd</translation>
    </message>
    <message>
        <source>Hostname or IP of your proxy server</source>
        <translation>Värdnamn eller IP-adress till din proxyserver</translation>
    </message>
    <message>
        <source>Port</source>
        <translation>Port</translation>
    </message>
    <message>
        <source>Username</source>
        <translation>Användarnamn</translation>
    </message>
    <message>
        <source>Your username for proxy server authentication</source>
        <translation>Ditt användarnamn för proxyserverns autentisering</translation>
    </message>
    <message>
        <source>Password</source>
        <translation>Lösenord</translation>
    </message>
    <message>
        <source>Your password for proxy server authentication</source>
        <translation>Ditt lösenord för proxyserverns autentisering</translation>
    </message>
    <message>
        <source>Display password</source>
        <translation>Visa lösenordet</translation>
    </message>
    <message>
        <source>Code</source>
        <extracomment>Lang. code column of language list.</extracomment>
        <translation>Språkkod</translation>
    </message>
    <message>
        <source>Version</source>
        <extracomment>Version column of skin list.</extracomment>
        <translation>Version</translation>
    </message>
    <message>
        <source>Author</source>
        <translation>Översättare</translation>
    </message>
    <message>
        <source>Email</source>
        <translation>E-post</translation>
    </message>
    <message>
        <source>Socks5</source>
        <translation>Socks5</translation>
    </message>
    <message>
        <source>Http</source>
        <translation>HTTP</translation>
    </message>
    <message>
        <source> (not supported on this platform)</source>
        <translation> (stöds inte på den här plattformen)</translation>
    </message>
    <message>
        <source>Tray area &amp;&amp; notifications</source>
        <translation>Meddelandefält &amp;&amp; notifieringar</translation>
    </message>
    <message>
        <source>Disable</source>
        <translation>Inaktivera</translation>
    </message>
    <message>
        <source>Enable</source>
        <translation>Aktivera</translation>
    </message>
    <message>
        <source>Tabs</source>
        <translation>Flikar</translation>
    </message>
    <message>
        <source>Close tabs with</source>
        <translation>Stäng flikar med</translation>
    </message>
    <message>
        <source>Middle mouse button single-click</source>
        <translation>Enkelklick på mushjulsknappen</translation>
    </message>
    <message>
        <source>Open new tabs with left mouse button double-click on tab bar</source>
        <translation>Öppna nya flikar med dubbelklick i flikfältet</translation>
    </message>
    <message>
        <source>Enable mouse gestures</source>
        <translation>Aktivera musgester</translation>
    </message>
    <message>
        <source>Web browser &amp; proxy</source>
        <translation>Webbläsare &amp; Proxy</translation>
    </message>
    <message>
        <source>Disable (Tray icon is not available.)</source>
        <translation>Inaktivera (ingen medelandefältsikon)</translation>
    </message>
    <message>
        <source>Queue new tabs (with hyperlinks) after the active tab</source>
        <translation>Öppna nya flikar (med hyperlänkar) efter aktuell flik</translation>
    </message>
    <message>
        <source>no icon theme</source>
        <extracomment>Label for disabling icon theme.</extracomment>
        <translation>Inget ikontema</translation>
    </message>
    <message>
        <source>Cannot save settings</source>
        <translation>Kan inte spara inställningar</translation>
    </message>
    <message>
        <source>Name</source>
        <extracomment>Skin list name column.</extracomment>
        <translation>Namn</translation>
    </message>
    <message>
        <source>Icons</source>
        <translation>Ikoner</translation>
    </message>
    <message>
        <source>Skins</source>
        <translation>Teman</translation>
    </message>
    <message>
        <source>Active skin:</source>
        <translation>Aktivt tema:</translation>
    </message>
    <message>
        <source>Selected skin:</source>
        <translation>Valt tema:</translation>
    </message>
    <message>
        <source>Hide tab bar if just one tab is visible</source>
        <translation>Dölj flikfältet om endast en flik är öppen</translation>
    </message>
    <message>
        <source>Critical settings were changed</source>
        <translation>Kritiska inställningar har ändrats</translation>
    </message>
    <message>
        <source>Feeds &amp; messages</source>
        <translation>Flöden &amp; meddelanden</translation>
    </message>
    <message>
        <source>Some critical settings are not set. You must fix these settings in order confirm new settings.</source>
        <translation>Vissa kritiska inställningar har inte konfigurerats. Du måste åtgärda dessa för att kunna verkställa inställningarna.</translation>
    </message>
    <message>
        <source>Messages</source>
        <translation>Meddelanden</translation>
    </message>
    <message>
        <source>Web browser executable</source>
        <translation>Startfil för webbläsare</translation>
    </message>
    <message>
        <source>...</source>
        <translation>...</translation>
    </message>
    <message>
        <source>Executable parameters</source>
        <translation>Startparametrar</translation>
    </message>
    <message>
        <source>Note that &quot;%1&quot; (without quotation marks) is placeholder for URL of selected message.</source>
        <translation>Notera att &quot;%1&quot; (utan citationstecken) är platshållare för det markerade meddelandets URL.</translation>
    </message>
    <message>
        <source>Select web browser executable</source>
        <translation>Välj startfil för webbläsaren</translation>
    </message>
    <message>
        <source>Executables (*.*)</source>
        <extracomment>File filter for external browser selection dialog.</extracomment>
        <translation>exe-filer (*.*)</translation>
    </message>
    <message>
        <source>Opera 12 or older</source>
        <translation>Opera 12 eller äldre</translation>
    </message>
    <message>
        <source>Executable file of web browser</source>
        <translation>Webbläsarens startfil</translation>
    </message>
    <message>
        <source>Parameters to executable</source>
        <translation>Parametrar för startfilen</translation>
    </message>
    <message>
        <source>some keyboard shortcuts are not unique</source>
        <translation>Vissa tangentbordsgenvägar är inte unika</translation>
    </message>
    <message>
        <source>List of errors:
%1.</source>
        <translation>Lista över fel:
%1.</translation>
    </message>
    <message>
        <source>List of changes:
%1.</source>
        <translation>Lista över ändringar:
%1.</translation>
    </message>
    <message>
        <source>language changed</source>
        <translation>Språket har ändrats</translation>
    </message>
    <message>
        <source>icon theme changed</source>
        <translation>Ikontemat har ändrats</translation>
    </message>
    <message>
        <source>skin changed</source>
        <translation>Temat har ändrats</translation>
    </message>
    <message>
        <source>Use sample arguments for</source>
        <translation>Använd argument för</translation>
    </message>
    <message>
        <source>Use in-memory database as the working database</source>
        <translation>Använd minnesdatabas (IMDB) som arbetsdatabas</translation>
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
        <translation>Användning av minnesdatabas har åtskilliga fördelar och fallgropar. Se till att du känner till dessa innan du aktiverar den här funktionen.
Fördelar:
&lt;ul&gt;
&lt;li&gt;Bättre hastighet för manipulation av flöden/meddelanden(speciellt vid visning av tusentals meddelanden).&lt;/li&gt;
&lt;li&gt;Hela databasen lagrad i RAM, för en mindre belastad hårddisk.&lt;/li&gt;
&lt;/ul&gt;
Nackdelar:
&lt;ul&gt;
&lt;li&gt;Om programmet kraschar, förlorar du ändringar gjorda under den sessionen.&lt;/li&gt;
&lt;li&gt;Programstart och avslut kan ta lite längre tid (max. 2 sekunder).&lt;/li&gt;
&lt;/ul&gt;
Utvecklaren av detta program, är INTE ansvarig för förlorad data.</translation>
    </message>
    <message>
        <source>in-memory database switched</source>
        <translation>Minnesdatabas växlad</translation>
    </message>
    <message>
        <source>Internal web browser</source>
        <translation>Intern webbläsare</translation>
    </message>
    <message>
        <source>External web browser</source>
        <translation>Extern webbläsare</translation>
    </message>
    <message>
        <source>Remove all read messages from all standard feeds on application exit</source>
        <translation>Ta bort alla lästa meddelanden från samtliga flöden vid programavslut</translation>
    </message>
    <message>
        <source>WARNING: Note that switching to another data storage type will NOT copy existing your data from currently active data storage to newly selected one.</source>
        <translation>VARNING! Notera att byte till en annan datalagringstyp INTE kopierar befintliga data från den aktiva datalagringen till den nyvalda.</translation>
    </message>
    <message>
        <source>Database driver</source>
        <translation>Databasdrivrutin</translation>
    </message>
    <message>
        <source>Hostname</source>
        <translation>Värdnamn</translation>
    </message>
    <message>
        <source>Test setup</source>
        <translation>Testa</translation>
    </message>
    <message>
        <source>Note that speed of used MySQL server and latency of used connection medium HEAVILY influences the final performance of this application. Using slow database connections leads to bad performance when browsing feeds or messages.</source>
        <translation>Notera att hastigheten på den använda MySQL-servern och latensen för aktuellt anslutningsmedium, KRAFTIGT påverkar detta programs prestanda. Användning av långsamma databasanslutningar leder till dålig prestanda vid navigering bland flöden och meddelanden.</translation>
    </message>
    <message>
        <source>Right mouse button double-click</source>
        <translation>Dubbelklick på höger musknapp</translation>
    </message>
    <message>
        <source>Auto-update all feeds every</source>
        <translation>Auto-uppdatera alla flöden varje</translation>
    </message>
    <message>
        <source> minutes</source>
        <translation>minuter</translation>
    </message>
    <message>
        <source>Feed connection timeout</source>
        <translation>Anslutnings-timeout för flöden</translation>
    </message>
    <message>
        <source>Connection timeout is time interval which is reserved for downloading new messages for the feed. If this time interval elapses, then download process is aborted.</source>
        <translation>Anslutnings-timeout är det tidsintervall som reserverats föratt ladda ner nya meddelanden. Om detta tidsintervall överskrids, kommer nedladdningsprocessen att avbrytas.</translation>
    </message>
    <message>
        <source> ms</source>
        <translation> ms</translation>
    </message>
    <message>
        <source>Update all feed on application startup</source>
        <translation>Uppdatera alla flöden vid programstart</translation>
    </message>
    <message>
        <source>Data storage</source>
        <translation>Datalagring</translation>
    </message>
    <message>
        <source>SQLite (embedded database)</source>
        <translation>SQLite (inbäddad databas)</translation>
    </message>
    <message>
        <source>MySQL/MariaDB (dedicated database)</source>
        <translation>MySQL/MariaDB (dedikerad databas)</translation>
    </message>
    <message>
        <source>Hostname of your MySQL server</source>
        <translation>Värdnamn för din MySQL-server</translation>
    </message>
    <message>
        <source>Username to login with</source>
        <translation>Användarnamn för att logga in</translation>
    </message>
    <message>
        <source>Password for your username</source>
        <translation>Lösenord för ditt användarnamn</translation>
    </message>
    <message>
        <source>data storage backend changed</source>
        <translation>Datalagringens stöddatabas har ändrats</translation>
    </message>
    <message>
        <source>Hostname is empty.</source>
        <translation>Värdnamn saknas.</translation>
    </message>
    <message>
        <source>Hostname looks ok.</source>
        <translation>Värdnamnet ser ok ut.</translation>
    </message>
    <message>
        <source>Username is empty.</source>
        <translation>Användarnamn saknas.</translation>
    </message>
    <message>
        <source>Username looks ok.</source>
        <translation>Användarnamnet ser ok ut.</translation>
    </message>
    <message>
        <source>Password is empty.</source>
        <translation>Lösenord saknas.</translation>
    </message>
    <message>
        <source>Password looks ok.</source>
        <translation>Lösenordet ser ok ut.</translation>
    </message>
    <message>
        <source>Toolbar button style</source>
        <translation>Verktygsfältets knappstil</translation>
    </message>
    <message>
        <source>Hide main window when it is minimized</source>
        <translation>Dölj programfönstret vid minimering</translation>
    </message>
    <message>
        <source>No connection test triggered so far.</source>
        <translation>Inget anslutningstest utfört än.</translation>
    </message>
    <message>
        <source>Note that these settings are applied only on newly established connections.</source>
        <translation>Notera att dess inställningar endast verkställs för nyetablerade anslutningar.</translation>
    </message>
    <message>
        <source>Select browser</source>
        <translation>Välj webbläsare</translation>
    </message>
    <message>
        <source>No proxy</source>
        <translation>Ingen proxy</translation>
    </message>
    <message>
        <source>System proxy</source>
        <translation>Systemets proxy</translation>
    </message>
    <message>
        <source>Icon only</source>
        <translation>Endast ikon</translation>
    </message>
    <message>
        <source>Text only</source>
        <translation>Endast text</translation>
    </message>
    <message>
        <source>Text beside icon</source>
        <translation>Text bredvid ikon</translation>
    </message>
    <message>
        <source>Text under icon</source>
        <translation>Text under ikon</translation>
    </message>
    <message>
        <source>Follow OS style</source>
        <translation>Operativsystemets stil</translation>
    </message>
    <message>
        <source>Keep message selection in the middle of the message list viewport</source>
        <translation>Placera markerat meddelande i mitten av vyn för meddelandelista</translation>
    </message>
    <message>
        <source>You did not executed any connection test yet.</source>
        <translation>Du har inte utfört något anslutningstest än.</translation>
    </message>
    <message>
        <source>Launch %1 on operating system startup</source>
        <translation>Starta %1 vid systemstart</translation>
    </message>
    <message>
        <source>Mouse gestures work with middle mouse button. Possible gestures are:
&lt;ul&gt;
&lt;li&gt;previous web page (drag mouse left),&lt;/li&gt;
&lt;li&gt;next web page (drag mouse right),&lt;/li&gt;
&lt;li&gt;reload current web page (drag mouse up),&lt;/li&gt;
&lt;li&gt;open new web browser tab (drag mouse down).&lt;/li&gt;
&lt;/ul&gt;</source>
        <translation>Musgester aktiveras med mushjulsknappen. Aktuella musgester är:
&lt;ul&gt;
&lt;li&gt;Föregående sida (dra musen åt vänster),&lt;/li&gt;
&lt;li&gt;Nästa sida (dra musen åt höger),&lt;/li&gt;
&lt;li&gt;Uppdatera aktuell webbsida (dra musen uppåt),&lt;/li&gt;
&lt;li&gt;Öppna ny webbläsarflik (dra musen neråt).&lt;/li&gt;
&lt;/ul&gt;</translation>
    </message>
    <message>
        <source>Enable JavaScript</source>
        <translation>Aktivera JavaScript</translation>
    </message>
    <message>
        <source>Enable external plugins based on NPAPI</source>
        <translation>Aktivera externa tillägg baserade på NPAPI</translation>
    </message>
    <message>
        <source>Auto-load images</source>
        <translation>Läs in bilder automatiskt</translation>
    </message>
    <message>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;If unchecked, then default system-wide web browser is used.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Om kryssrutan lämnas omarkerad, används systemets standardwebbläsare.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <source>Custom external web browser</source>
        <translation>Anpassad extern webbläsare</translation>
    </message>
    <message>
        <source>Feeds &amp;&amp; categories</source>
        <translation>Flöden &amp;&amp; kategorier</translation>
    </message>
    <message>
        <source>Message count format in feed list</source>
        <translation>Meddelanderäknare för flödeslistan</translation>
    </message>
    <message>
        <source>Enter format for count of messages displayed next to each feed/category in feed list. Use &quot;%all&quot; and &quot;%unread&quot; strings which are placeholders for the actual count of all (or unread) messages.</source>
        <translation>Ange vilket format meddelanderäknaren skall visa antal meddelanden förvarje flöde/kategori. Använd &quot;%all&quot; och &quot;%unread&quot; vilket är platshållare för alla, respektive olästa meddelanden.</translation>
    </message>
    <message>
        <source>custom external browser is not set correctly</source>
        <translation>Anpassad extern webbläsare är inte korrekt konfigurerad</translation>
    </message>
    <message>
        <source>Toolbars</source>
        <translation>Verktygsfält</translation>
    </message>
    <message>
        <source>Toolbar for feeds list</source>
        <translation>Verktygsfält för flödeslista</translation>
    </message>
    <message>
        <source>Toolbar for messages list</source>
        <translation>Verktygsfält för meddelandelista</translation>
    </message>
    <message>
        <source>Select toolbar to edit</source>
        <translation>Välj verktygsfält att redigera</translation>
    </message>
    <message>
        <source>Some critical settings were changed and will be applied after the application gets restarted. 

You have to restart manually.</source>
        <translation>Vissa kritiska inställningar har ändrats och kommer att verkställas efter att programmet har startats om. 

Du måste starta om manuellt.</translation>
    </message>
    <message>
        <source>Do you want to restart now?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Check for updates on application startup</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Remove junk Trolltech registry key (HKCUSoftwareTrolltech) whn application quits (Use at your own risk!)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Use custom date/time format (overrides format loaded from active localization)</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>FormUpdate</name>
    <message>
        <source>Current release</source>
        <translation>Aktuell version</translation>
    </message>
    <message>
        <source>Available release</source>
        <translation>Tillgänglig version</translation>
    </message>
    <message>
        <source>Changes</source>
        <translation>Ändringar</translation>
    </message>
    <message>
        <source>Status</source>
        <translation>Status</translation>
    </message>
    <message>
        <source>unknown</source>
        <extracomment>Unknown release.</extracomment>
        <translation>Okänd</translation>
    </message>
    <message>
        <source>List with updates was not
downloaded successfully.</source>
        <translation>Listan med uppdateringar
laddades inte ner korrekt.</translation>
    </message>
    <message>
        <source>New release available.</source>
        <translation>Ny version tillgänglig.</translation>
    </message>
    <message>
        <source>This is new version which can be
downloaded and installed.</source>
        <translation>Ny version som kan laddas
ner och installeras.</translation>
    </message>
    <message>
        <source>Error: &apos;%1&apos;.</source>
        <translation>Fel: &apos;%1&apos;.</translation>
    </message>
    <message>
        <source>No new release available.</source>
        <translation>Ingen ny version tillgänglig.</translation>
    </message>
    <message>
        <source>This release is not newer than
currently installed one.</source>
        <translation>Den här versionen är nyare än den installerade.</translation>
    </message>
    <message>
        <source>Check for updates</source>
        <translation>Sök efter uppdateringar</translation>
    </message>
    <message>
        <source>Update</source>
        <translation>Uppdatera</translation>
    </message>
    <message>
        <source>Download new installation files.</source>
        <translation>Ladda ner ny installationsfil.</translation>
    </message>
    <message>
        <source>Checking for updates failed.</source>
        <translation>Kunde inte söka efter uppdatering.</translation>
    </message>
    <message>
        <source>Download installation file for your OS.</source>
        <translation>Ladda ner installationsfil för ditt system.</translation>
    </message>
    <message>
        <source>Installation file is not available directly.
Go to application website to obtain it manually.</source>
        <translation>Installationsfilen är inte direkt tillgänglig.
Gå till programmets hemsida för att hämta den manuellt.</translation>
    </message>
    <message>
        <source>No new update available.</source>
        <translation>Ingen ny uppdatering tillgänglig.</translation>
    </message>
    <message>
        <source>Cannot update application</source>
        <translation>Kan inte uppdatera programmet</translation>
    </message>
    <message>
        <source>Cannot navigate to installation file. Check new installation downloads manually on project website.</source>
        <translation>Kan inte navigera till installationsfilen. Sök ny installationsnedladdning manuellt, på projektets hemsida.</translation>
    </message>
    <message>
        <source>Download update</source>
        <translation>Ladda ner uppdatering</translation>
    </message>
    <message>
        <source>Downloaded %1% (update size is %2 kB).</source>
        <translation>%1% nerladdat (uppdateringens storlek är %2 kB).</translation>
    </message>
    <message>
        <source>Downloading update...</source>
        <translation>Laddar ner uppdatering...</translation>
    </message>
    <message>
        <source>Downloaded successfully</source>
        <translation>Nedladdning slutförd</translation>
    </message>
    <message>
        <source>Package was downloaded successfully.</source>
        <translation>Paketet har laddats ner korrekt.</translation>
    </message>
    <message>
        <source>Install update</source>
        <translation>Installera uppdatering</translation>
    </message>
    <message>
        <source>Error occured</source>
        <translation>Ett fel uppstod</translation>
    </message>
    <message>
        <source>Error occured during downloading of the package.</source>
        <translation>Ett fel uppstod vid nedladdningen.</translation>
    </message>
    <message>
        <source>Cannot launch external updater. Update application manually.</source>
        <translation>Kan inte starta den externa uppdateraren. Uppdatera programmet manuellt.</translation>
    </message>
    <message>
        <source>Go to application website</source>
        <translation>Gå till programmets hemsida</translation>
    </message>
</context>
<context>
    <name>LocationLineEdit</name>
    <message>
        <source>Website address goes here</source>
        <translation>Webbadress anges här</translation>
    </message>
</context>
<context>
    <name>MessagesModel</name>
    <message>
        <source>Id</source>
        <extracomment>Tooltip for ID of message.</extracomment>
        <translation>ID</translation>
    </message>
    <message>
        <source>Read</source>
        <extracomment>Tooltip for &quot;read&quot; column in msg list.</extracomment>
        <translation>Läst</translation>
    </message>
    <message>
        <source>Deleted</source>
        <extracomment>Tooltip for &quot;deleted&quot; column in msg list.</extracomment>
        <translation>Borttaget</translation>
    </message>
    <message>
        <source>Important</source>
        <extracomment>Tooltip for &quot;important&quot; column in msg list.</extracomment>
        <translation>Viktigt</translation>
    </message>
    <message>
        <source>Feed</source>
        <extracomment>Tooltip for name of feed for message.</extracomment>
        <translation>Flöde</translation>
    </message>
    <message>
        <source>Title</source>
        <extracomment>Tooltip for title of message.</extracomment>
        <translation>Titel</translation>
    </message>
    <message>
        <source>Url</source>
        <extracomment>Tooltip for url of message.</extracomment>
        <translation>URL</translation>
    </message>
    <message>
        <source>Author</source>
        <extracomment>Tooltip for author of message.</extracomment>
        <translation>Författare</translation>
    </message>
    <message>
        <source>Created on</source>
        <extracomment>Tooltip for creation date of message.</extracomment>
        <translation>Skapad</translation>
    </message>
    <message>
        <source>Contents</source>
        <extracomment>Tooltip for contents of message.</extracomment>
        <translation>Innehåll</translation>
    </message>
    <message>
        <source>Id of the message.</source>
        <translation>Meddelande-ID.</translation>
    </message>
    <message>
        <source>Is message read?</source>
        <translation>Är meddelandet läst?</translation>
    </message>
    <message>
        <source>Is message deleted?</source>
        <translation>Är meddelandet borttaget?</translation>
    </message>
    <message>
        <source>Is message important?</source>
        <translation>Är meddelandet viktigt?</translation>
    </message>
    <message>
        <source>Id of feed which this message belongs to.</source>
        <translation>ID för det flöde som detta meddelande tillhör.</translation>
    </message>
    <message>
        <source>Title of the message.</source>
        <translation>Meddelandetitel.</translation>
    </message>
    <message>
        <source>Url of the message.</source>
        <translation>URL för meddelandet.</translation>
    </message>
    <message>
        <source>Author of the message.</source>
        <translation>Meddelandets författare.</translation>
    </message>
    <message>
        <source>Creation date of the message.</source>
        <translation>Skapelsedatum för meddelandet.</translation>
    </message>
    <message>
        <source>Contents of the message.</source>
        <translation>Innehåll i meddelandet.</translation>
    </message>
</context>
<context>
    <name>MessagesToolBar</name>
    <message>
        <source>Search messages</source>
        <translation>Sök meddelande</translation>
    </message>
    <message>
        <source>Message search box</source>
        <translation>Sökfält</translation>
    </message>
    <message>
        <source>Menu for highlighting messages</source>
        <translation>Meny för färgmarkering av meddelanden</translation>
    </message>
    <message>
        <source>No extra highlighting</source>
        <translation>Ingen färgmarkering</translation>
    </message>
    <message>
        <source>Highlight unread messages</source>
        <translation>Färgmarkera olästa meddelanden</translation>
    </message>
    <message>
        <source>Highlight important messages</source>
        <translation>Färgmarkera viktiga meddelanden</translation>
    </message>
    <message>
        <source>Display all messages</source>
        <translation>Visa alla meddelande</translation>
    </message>
    <message>
        <source>Message highlighter</source>
        <translation>Färgmarkör</translation>
    </message>
    <message>
        <source>Toolbar spacer</source>
        <translation>Verktygsavskiljare</translation>
    </message>
</context>
<context>
    <name>MessagesView</name>
    <message>
        <source>Context menu for messages</source>
        <translation>Kontextmeny för meddelanden</translation>
    </message>
    <message>
        <source>Meesage without URL</source>
        <translation>Meddelande utan URL</translation>
    </message>
    <message>
        <source>Message &apos;%s&apos; does not contain URL.</source>
        <translation>Meddelande &apos;%s&apos; innehåller ingen URL.</translation>
    </message>
    <message>
        <source>Problem with starting external web browser</source>
        <translation>Problem med att starta extern webbläsare</translation>
    </message>
    <message>
        <source>External web browser could not be started.</source>
        <translation>Extern webbläsare kan inte startas.</translation>
    </message>
</context>
<context>
    <name>NetworkFactory</name>
    <message>
        <source>protocol error</source>
        <extracomment>Network status.</extracomment>
        <translation>Protokollfel</translation>
    </message>
    <message>
        <source>host not found</source>
        <extracomment>Network status.</extracomment>
        <translation>Värddatorn kan inte hittas</translation>
    </message>
    <message>
        <source>connection refused</source>
        <extracomment>Network status.</extracomment>
        <translation>Anslutning nekades</translation>
    </message>
    <message>
        <source>connection timed out</source>
        <extracomment>Network status.</extracomment>
        <translation>Anslutningstiden överskreds</translation>
    </message>
    <message>
        <source>SSL handshake failed</source>
        <extracomment>Network status.</extracomment>
        <translation>SSL-handskakning misslyckades</translation>
    </message>
    <message>
        <source>proxy server connection refused</source>
        <extracomment>Network status.</extracomment>
        <translation>Proxy-anslutning nekades</translation>
    </message>
    <message>
        <source>temporary failure</source>
        <extracomment>Network status.</extracomment>
        <translation>Temporärt fel</translation>
    </message>
    <message>
        <source>authentication failed</source>
        <extracomment>Network status.</extracomment>
        <translation>Autentiseringen misslyckades</translation>
    </message>
    <message>
        <source>proxy authentication required</source>
        <extracomment>Network status.</extracomment>
        <translation>Proxyautentisering krävs</translation>
    </message>
    <message>
        <source>proxy server not found</source>
        <extracomment>Network status.</extracomment>
        <translation>Proxyservern hittades inte</translation>
    </message>
    <message>
        <source>uknown content</source>
        <extracomment>Network status.</extracomment>
        <translation>Okänt innehåll</translation>
    </message>
    <message>
        <source>content not found</source>
        <extracomment>Network status.</extracomment>
        <translation>Inget innehåll hittades</translation>
    </message>
    <message>
        <source>unknown error</source>
        <extracomment>Network status.</extracomment>
        <translation>Okänt fel</translation>
    </message>
    <message>
        <source>no errors</source>
        <extracomment>Network status.</extracomment>
        <translation>Inga fel</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <source>LANG_NAME</source>
        <extracomment>Name of language, e.g. English.</extracomment>
        <translation>Swedish</translation>
    </message>
    <message>
        <source>LANG_ABBREV</source>
        <extracomment>Abbreviation of language, e.g. en. Use ISO 639-1 code here combined with ISO 3166-1 (alpha-2) code. Examples: &quot;cs_CZ&quot;, &quot;en_GB&quot;, &quot;en_US&quot;.</extracomment>
        <translation>sv_SE</translation>
    </message>
    <message>
        <source>LANG_VERSION</source>
        <extracomment>Version of your translation, e.g. 1.0.</extracomment>
        <translation>2.0.0.2</translation>
    </message>
    <message>
        <source>LANG_AUTHOR</source>
        <extracomment>Name of translator - optional.</extracomment>
        <translation>Åke Engelbrektson</translation>
    </message>
    <message>
        <source>LANG_EMAIL</source>
        <extracomment>Email of translator - optional.</extracomment>
        <translation>eson57@gmail.com</translation>
    </message>
</context>
<context>
    <name>ShortcutCatcher</name>
    <message>
        <source>Reset to original shortcut.</source>
        <translation>Återställ ursprunglig ikon.</translation>
    </message>
    <message>
        <source>Clear current shortcut.</source>
        <translation>Rensa aktuell genväg.</translation>
    </message>
    <message>
        <source>Click and hit new shortcut.</source>
        <translation>Klicka och välj ny snabbtangent.</translation>
    </message>
</context>
<context>
    <name>StatusBar</name>
    <message>
        <source>Fullscreen mode</source>
        <translation>Fullskärmsläge</translation>
    </message>
    <message>
        <source>Switch application between fulscreen/normal states right from this status bar icon.</source>
        <translation>Växla mellan fullskärmsläge och fönsterläge.</translation>
    </message>
</context>
<context>
    <name>SystemFactory</name>
    <message>
        <source>New version available</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Click the bubble for more information.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>SystemTrayIcon</name>
    <message>
        <source>%1
Unread news: %2</source>
        <translation>%1
Olästa nyheter: %2</translation>
    </message>
</context>
<context>
    <name>TabBar</name>
    <message>
        <source>Close this tab.</source>
        <translation>Stäng den här fliken.</translation>
    </message>
    <message>
        <source>Close tab</source>
        <translation>Stäng fliken</translation>
    </message>
</context>
<context>
    <name>TabWidget</name>
    <message>
        <source>Feeds</source>
        <translation>Flöden</translation>
    </message>
    <message>
        <source>Browse your feeds and messages</source>
        <translation>Bläddra bland dina flöden och meddelanden</translation>
    </message>
    <message>
        <source>Web browser</source>
        <extracomment>Web browser default tab title.</extracomment>
        <translation>Webbläsare</translation>
    </message>
    <message>
        <source>Displays main menu.</source>
        <translation>Visar den primära menyn.</translation>
    </message>
    <message>
        <source>Main menu</source>
        <translation>Huvudmeny</translation>
    </message>
    <message>
        <source>Open new web browser tab.</source>
        <translation>Öppna ny webbläsarflik.</translation>
    </message>
</context>
<context>
    <name>ToolBarEditor</name>
    <message>
        <source>Activated actions</source>
        <translation>Aktiverade åtgärder</translation>
    </message>
    <message>
        <source>Available actions</source>
        <translation>Tillgängliga åtgärder</translation>
    </message>
    <message>
        <source>Insert separator</source>
        <translation>Infoga separator</translation>
    </message>
    <message>
        <source>Insert spacer</source>
        <translation>Infoga mellanrum</translation>
    </message>
    <message>
        <source>Separator</source>
        <translation>Separator</translation>
    </message>
    <message>
        <source>Toolbar spacer</source>
        <translation>Mellanrum</translation>
    </message>
</context>
<context>
    <name>TrayIconMenu</name>
    <message>
        <source>Close opened modal dialogs first.</source>
        <translation>Stäng öppna dialogrutor först.</translation>
    </message>
</context>
<context>
    <name>WebBrowser</name>
    <message>
        <source>Navigation panel</source>
        <translation>Navigationspanel</translation>
    </message>
    <message>
        <source>Back</source>
        <translation>Tillbaka</translation>
    </message>
    <message>
        <source>Forward</source>
        <translation>Framåt</translation>
    </message>
    <message>
        <source>Reload</source>
        <translation>Uppdatera</translation>
    </message>
    <message>
        <source>Stop</source>
        <translation>Stopp</translation>
    </message>
    <message>
        <source>Zoom  </source>
        <translation>Zoom </translation>
    </message>
    <message>
        <source>No title</source>
        <extracomment>Webbrowser tab title when no title is available.</extracomment>
        <translation>Ingen titel</translation>
    </message>
    <message>
        <source>Decrease zoom.</source>
        <translation>Mindre zoom.</translation>
    </message>
    <message>
        <source>Reset zoom to default.</source>
        <translation>Återställ zoomläget till standard.</translation>
    </message>
    <message>
        <source>Increase zoom.</source>
        <translation>Ökad zoom.</translation>
    </message>
    <message>
        <source>Written by </source>
        <translation>Skriven av </translation>
    </message>
    <message>
        <source>uknown author</source>
        <translation>okänd författare</translation>
    </message>
    <message>
        <source>Newspaper view</source>
        <translation>Tidningsvy</translation>
    </message>
    <message>
        <source>Go back.</source>
        <translation>Gå tillbaka.</translation>
    </message>
    <message>
        <source>Go forward.</source>
        <translation>Gå framåt.</translation>
    </message>
    <message>
        <source>Reload current web page.</source>
        <translation>Uppdatera aktuell webbsida.</translation>
    </message>
    <message>
        <source>Stop web page loading.</source>
        <translation>Stoppa inläsning av webbsidan.</translation>
    </message>
</context>
<context>
    <name>WebView</name>
    <message>
        <source>Reload web page</source>
        <translation>Uppdatera webbsidan</translation>
    </message>
    <message>
        <source>Copy link url</source>
        <translation>Kopiera länk</translation>
    </message>
    <message>
        <source>Copy image</source>
        <translation>Kopiera bild</translation>
    </message>
    <message>
        <source>Copy image url</source>
        <translation>Kopiera bildadress</translation>
    </message>
    <message>
        <source>Open link in new tab</source>
        <translation>Öppna länk i ny flik</translation>
    </message>
    <message>
        <source>Follow link</source>
        <translation>Följ länk</translation>
    </message>
    <message>
        <source>Open image in new tab</source>
        <translation>Öppna bild i ny flik</translation>
    </message>
    <message>
        <source>Page not found</source>
        <translation>Sidan kan inte hittas</translation>
    </message>
    <message>
        <source>Check your internet connection or website address</source>
        <translation>Kontrollera din Internetanslutning och/eller webbadressen</translation>
    </message>
    <message>
        <source>This failure can be caused by:&lt;br&gt;&lt;ul&gt;&lt;li&gt;non-functional internet connection,&lt;/li&gt;&lt;li&gt;incorrect website address,&lt;/li&gt;&lt;li&gt;bad proxy server settings,&lt;/li&gt;&lt;li&gt;target destination outage,&lt;/li&gt;&lt;li&gt;many other things.&lt;/li&gt;&lt;/ul&gt;</source>
        <translation>Detta fel kan ha orsakats av:&lt;br&gt;&lt;ul&gt;&lt;li&gt;Icke fungerande Internetanslutning,&lt;/li&gt;&lt;li&gt;felaktig webbadress,&lt;/li&gt;&lt;li&gt;felaktiga proxyserverinställningar,&lt;/li&gt;&lt;li&gt;strömavbrott på måldestinationen&lt;/li&gt;och/eller&lt;li&gt;många andra saker.&lt;/li&gt;&lt;/ul&gt;</translation>
    </message>
    <message>
        <source>Web browser</source>
        <translation>Webbläsare</translation>
    </message>
    <message>
        <source>Image</source>
        <translation>Bild</translation>
    </message>
    <message>
        <source>Hyperlink</source>
        <translation>Hyperlänk</translation>
    </message>
    <message>
        <source>Error page</source>
        <translation>Felsida</translation>
    </message>
    <message>
        <source>Reload current web page.</source>
        <translation>Uppdatera aktuell webbsida.</translation>
    </message>
    <message>
        <source>Copy selection</source>
        <translation>Kopiera markerat</translation>
    </message>
    <message>
        <source>Copies current selection into the clipboard.</source>
        <translation>Kopiera aktuell markering till Urklipp.</translation>
    </message>
    <message>
        <source>Copy link url to clipboard.</source>
        <translation>Kopiera länkadress till Urklipp.</translation>
    </message>
    <message>
        <source>Copy image to clipboard.</source>
        <translation>Kopiera bild till Urklipp.</translation>
    </message>
    <message>
        <source>Copy image url to clipboard.</source>
        <translation>Kopiera bildadress till Urklipp.</translation>
    </message>
    <message>
        <source>Open this hyperlink in new tab.</source>
        <translation>Öppna länken i ny flik.</translation>
    </message>
    <message>
        <source>Open the hyperlink in this tab.</source>
        <translation>Öppna länken i samma flik.</translation>
    </message>
    <message>
        <source>Open this image in this tab.</source>
        <translation>Öppna bilden i samma flik.</translation>
    </message>
    <message>
        <source>Open link in external browser</source>
        <translation>Öppna länk i extern webbläsare</translation>
    </message>
    <message>
        <source>Open the hyperlink in external browser.</source>
        <translation>Öppna länken i extern webbläsare.</translation>
    </message>
</context>
</TS>
