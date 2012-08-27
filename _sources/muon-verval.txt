.. include:: subst.inc

Meten met één detector: muon verval
===================================

Het histogram: tijden tot een paar duizend ns
---------------------------------------------
In dit experiment voeren we een meting uit met één muonbalk.  Kies voor een Lifetime instelling (derde knop). Het scherm hieronder is het startscherm. De meting is nog niet begonnen.
 
.. figure:: images/beginscherm-levensduurmeting.png
   :align: center
   :width: 600
    
De x-as van het diagram geeft opnieuw een **tijdsverschil** aan in nanoseconden; terwijl er met één balk wordt gemeten. Het gaat nu om het tijdsverschil tussen binnenkomst en verval van een muon. Beide gebeurtenissen geven een signaal.
  
De waarden op de x-as zijn nu wel veel groter (de ordegrootte is nu duizend ns ofwel een μs i.p.v. enkele ns). De gemiddelde leeftijd is voor een muon een stuk groter dan de reistijd om enkele dm’s of meters af te leggen in lucht.

Op de y-as staat het aantal muonen dat vervalt na een bepaalde verblijftijd in de scintillator

.. note:: Je meet bij dit onderzoek de verblijftijd van een muon in de scintillator. Dat is de tijd tussen binnenkomst van een muon en diens verval. Verblijftijd is niet hetzelfde als leeftijd. De voorgeschiedenis van een muon (de tijd tussen ontstaan en aankomst bij de detector) is voor elk muon anders. Toch blijkt de verdeling van verblijftijden zeer goede informatie op te leveren over de gemiddelde levensduur, ofwel de vervalconstante (of de halveringstijd) van muonen. Bespreek dit met je begeleider. 

Voer een oriënterende meting uit van enkele minuten. Je zult zien dat de hit-rate (het aantal detecties per seconde) nu veel lager is dan bij het onderzoek naar looptijden van muonen. 
	
Schaling van de x-as (Plot Zoom en Plot Offset). Let op het onderstaande:
* Het diagram schaalt automatisch. De hoogste waarde in het histogram bepaalt de schaal van de grafiek.  
* Voor kleine waarden van de tijd kan een grote piek ontstaan. Dat komt door de meetapparatuur. Kort na een puls ontstaat vaak een tweede door reflecties. Het muon lijkt dan na zeer korte tijd te vervallen. Dat is geen reële meting. 
* Stel de Plot Offset daarom in op ongeveer 100ns. 
	
.. note:: In de figuur hieronder is ingezoomd op dit effect. De uitschieter rond de 75 ns is geen reële meting voor de vervaltijd van muonen.	
	
.. figure:: images/lifetime-zoom-in.png
   :align: center
   :width: 400

.. note:: Wanneer je zeker weet dat de detector goed werkt, start dan een langdurige meting (minstens enkele uren).
   
.. note:: In het excel document bij deze handleiding staat gedetailleerde informatie over het verwerken van de data die door de software worden opgeslagen.
   
Interpretatie van de metingen
-----------------------------
Wanneer je het derde onderzoek hebt afgerond kun je antwoord geven op de volgende vragen.
* Hoe kun je de halveringstijd bepalen uit een exponentieel dalende functie? 
* Hoe is de muon levensduur gedefinieerd? Wat is de relatie tussen halveringstijd en levensduur van muonen?
* Welke metingen uit het histogram zijn ongeldig (hebben niets te maken met de verblijftijd van muonen in de scintillator)? Je kunt dat afleiden uit de vorm van de grafiek en een eventuele verplaatsing. 
* Welke *verklaring* kun je geven voor het bestaan van ongewenste detecties (metingen kort na elkaar die niet horen bij een muon verval)?
* Hoe groot is de kans dat je een foutieve meting zult aantreffen? Is deze kans groter bij kleine tijdsintervallen dan bij grote tijdsintervallen?
