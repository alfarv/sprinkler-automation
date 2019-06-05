const char MAIN_page[] PROGMEM = R"====(
<HTML>
	<HEAD>
		<TITLE>SPRINKLE CREDENTIALS</TITLE>
	</HEAD>
<BODY>
  <CENTER>
		<B>IP credentials </B>
		<FORM method="post">
			<br>

      Nombre de la red
      <input id="lanId" type="text" name="lanId" required><br>
      <br>
      Pasword de la red
      <input id="pass" type="text" name="pass" required><br>
      <br>
      <br>
      Direccion IP autom.
      <input id="isAuto" type="radio" name="isAuto" value="Automatic" checked onclick=AUto()>
      <input id="isAuto" type="radio" name="isAuto" value="Manual" onclick=AUto()><br>
      <br>
      Local IP
      <input id="ipEntry11" type="number" name="ipEntry11" min="1" max="255" disabled>
      <input id="ipEntry12" type="number" name="ipEntry12" min="0" max="255" disabled>
      <input id="ipEntry13" type="number" name="ipEntry13" min="0" max="255" disabled>
      <input id="ipEntry14" type="number" name="ipEntry14" min="0" max="255" disabled><br>
      <br>
      Gateway
      <input id="ipEntry21" type="number" name="ipEntry21" min="1" max="255" disabled>
      <input id="ipEntry22" type="number" name="ipEntry22" min="0" max="255" disabled>
      <input id="ipEntry23" type="number" name="ipEntry23" min="0" max="255" disabled>
      <input id="ipEntry24" type="number" name="ipEntry24" min="0" max="255" disabled><br>
      <br>
      Sub-net mask
      <input id="ipEntry31" type="number" name="ipEntry31" min="1" max="255" disabled>
      <input id="ipEntry32" type="number" name="ipEntry32" min="0" max="255" disabled>
      <input id="ipEntry33" type="number" name="ipEntry33" min="0" max="255" disabled>
      <input id="ipEntry34" type="number" name="ipEntry34" min="0" max="255" disabled><br>
      <br>
      <input type="submit">

	    <script>
        function AUto(){
          if(document.getElementById("isAuto").checked==true){
            document.getElementById("ipEntry11").disabled=true;
            document.getElementById("ipEntry12").disabled=true;
            document.getElementById("ipEntry13").disabled=true;
            document.getElementById("ipEntry14").disabled=true;
            document.getElementById("ipEntry21").disabled=true;
            document.getElementById("ipEntry22").disabled=true;
            document.getElementById("ipEntry23").disabled=true;
            document.getElementById("ipEntry24").disabled=true;
            document.getElementById("ipEntry31").disabled=true;
            document.getElementById("ipEntry32").disabled=true;
            document.getElementById("ipEntry33").disabled=true;
            document.getElementById("ipEntry34").disabled=true;
            }
          else{
            document.getElementById("ipEntry11").disabled=false;
            document.getElementById("ipEntry12").disabled=false;
            document.getElementById("ipEntry13").disabled=false;
            document.getElementById("ipEntry14").disabled=false;
            document.getElementById("ipEntry21").disabled=false;
            document.getElementById("ipEntry22").disabled=false;
            document.getElementById("ipEntry23").disabled=false;
            document.getElementById("ipEntry24").disabled=false;
            document.getElementById("ipEntry31").disabled=false;
            document.getElementById("ipEntry32").disabled=false;
            document.getElementById("ipEntry33").disabled=false;
            document.getElementById("ipEntry34").disabled=false;            
            }
        }
      </script>
      
		</FORM>
   </CENTER>
</BODY>
</HTML>
)====";
