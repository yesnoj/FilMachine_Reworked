import { useState, useRef, useCallback, useEffect } from "react";

function mkRng(seed) {
  let s = seed | 0;
  return function () {
    s = (s + 0x6d2b79f5) | 0;
    var t = Math.imul(s ^ (s >>> 15), 1 | s);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

var PALETTES = {
  cyberpunk:{name:"Cyberpunk",colors:["#0d0221","#0f084b","#26408b","#a6209e","#e83f6f","#ff5e5b"],text:"#f0e6ff",accent:"#e83f6f"},
  aurora:{name:"Aurora",colors:["#0b0c10","#1f2833","#0b3d4e","#11998e","#38ef7d","#2de2e6"],text:"#e0fff4",accent:"#38ef7d"},
  lava:{name:"Lava",colors:["#1a0a0a","#3d0c02","#8b1a00","#d4380d","#ff7a45","#ffc53d"],text:"#fff1e6",accent:"#ff7a45"},
  ocean:{name:"Deep Ocean",colors:["#020024","#030637","#0d1b6f","#1565c0","#42a5f5","#80deea"],text:"#e0f7fa",accent:"#42a5f5"},
  forest:{name:"Forest",colors:["#0a1a0a","#1b3a1b","#2d5a27","#4caf50","#81c784","#a5d6a7"],text:"#e8f5e9",accent:"#81c784"},
  sunset:{name:"Sunset",colors:["#1a0533","#4a0e4e","#8e244d","#cf4647","#f28a30","#f7d060"],text:"#fff3e0",accent:"#f28a30"},
  machinery:{name:"Machinery",colors:["#1a1a1a","#2e2e2e","#4a4a4a","#ffb300","#ff8f00","#f57c00"],text:"#fff8e1",accent:"#ffb300"},
  arctic:{name:"Arctic",colors:["#e8eaf6","#b0bec5","#78909c","#546e7a","#37474f","#263238"],text:"#263238",accent:"#0288d1"},
  neon:{name:"Neon",colors:["#0a0a0a","#1a0a2e","#2d1b69","#7c3aed","#a78bfa","#00ffaa"],text:"#e0e7ff",accent:"#00ffaa"},
  pastel:{name:"Pastel",colors:["#fce4ec","#f8bbd0","#ce93d8","#9fa8da","#80cbc4","#a5d6a7"],text:"#37474f",accent:"#7b1fa2"},
};

var LVGL_SIZES = [12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48];
var FONTS = {
  montserrat:{name:"Montserrat",group:"Built-in LVGL",sizes:LVGL_SIZES,lvgl:function(sz){return"&lv_font_montserrat_"+sz;},preview:"'Montserrat',sans-serif",setup:function(sz){return"Enable LV_FONT_MONTSERRAT_"+sz+" in lv_conf.h";}},
  unscii:{name:"Unscii (Pixel)",group:"Built-in LVGL",sizes:[8,16],lvgl:function(sz){return"&lv_font_unscii_"+sz;},preview:"'Press Start 2P',monospace",setup:function(sz){return"Enable LV_FONT_UNSCII_"+sz+" in lv_conf.h";}},
  roboto:{name:"Roboto",group:"External",sizes:LVGL_SIZES,lvgl:function(sz){return"&font_roboto_"+sz;},preview:"'Roboto',sans-serif",setup:function(sz){return"lv_font_conv --font Roboto-Bold.ttf -o font_roboto_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x20-0x7F";}},
  opensans:{name:"Open Sans",group:"External",sizes:LVGL_SIZES,lvgl:function(sz){return"&font_opensans_"+sz;},preview:"'Open Sans',sans-serif",setup:function(sz){return"lv_font_conv --font OpenSans-Bold.ttf -o font_opensans_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x20-0x7F";}},
  orbitron:{name:"Orbitron",group:"External",sizes:LVGL_SIZES,lvgl:function(sz){return"&font_orbitron_"+sz;},preview:"'Orbitron',sans-serif",setup:function(sz){return"lv_font_conv --font Orbitron-Bold.ttf -o font_orbitron_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x20-0x7F";}},
  exo2:{name:"Exo 2",group:"External",sizes:LVGL_SIZES,lvgl:function(sz){return"&font_exo2_"+sz;},preview:"'Exo 2',sans-serif",setup:function(sz){return"lv_font_conv --font Exo2-Bold.ttf -o font_exo2_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x20-0x7F";}},
  rajdhani:{name:"Rajdhani",group:"External",sizes:LVGL_SIZES,lvgl:function(sz){return"&font_rajdhani_"+sz;},preview:"'Rajdhani',sans-serif",setup:function(sz){return"lv_font_conv --font Rajdhani-Bold.ttf -o font_rajdhani_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x20-0x7F";}},
  ubuntu:{name:"Ubuntu",group:"External",sizes:LVGL_SIZES,lvgl:function(sz){return"&font_ubuntu_"+sz;},preview:"'Ubuntu',sans-serif",setup:function(sz){return"lv_font_conv --font Ubuntu-Bold.ttf -o font_ubuntu_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x20-0x7F";}},
  sourcecodepro:{name:"Source Code Pro",group:"External",sizes:LVGL_SIZES,lvgl:function(sz){return"&font_sourcecodepro_"+sz;},preview:"'Source Code Pro',monospace",setup:function(sz){return"lv_font_conv --font SourceCodePro-Bold.ttf -o font_sourcecodepro_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x20-0x7F";}},
  custom:{name:"Custom Font",group:"Custom",sizes:LVGL_SIZES,lvgl:function(sz,n){return"&"+n+"_"+sz;},preview:"'Segoe UI',sans-serif",setup:function(sz,n){return"lv_font_conv --font YOUR_FONT.ttf -o "+n+"_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x20-0x7F";}},
};

var FONT_WEIGHTS = [
  {id:"normal",name:"Regular",css:400},
  {id:"bold",name:"Bold",css:700},
  {id:"extrabold",name:"Extra Bold",css:800},
  {id:"black",name:"Black",css:900},
];

function parseUnicode(input) {
  if (!input || !input.trim()) return null;
  var hex = input.trim().toLowerCase().replace(/^(0x|u\+|\\u|&#x)/i,"").replace(/;$/,"");
  if (/^[0-9a-f]{2,5}$/i.test(hex)) {
    var code = parseInt(hex, 16);
    if (code > 0) return {code:code, hex:hex.toUpperCase().padStart(4,"0"), char:String.fromCodePoint(code), escape:"\\u"+hex.padStart(4,"0").toUpperCase()};
  }
  return null;
}

// Font loading
if (typeof document !== "undefined") {
  if (!document.getElementById("gf-splash")) {
    var l = document.createElement("link");
    l.id = "gf-splash"; l.rel = "stylesheet";
    l.href = "https://fonts.googleapis.com/css2?family=Montserrat:wght@400;700;800;900&family=Roboto:wght@400;700;900&family=Open+Sans:wght@400;700;800&family=Rajdhani:wght@400;600;700&family=Orbitron:wght@400;700;900&family=Exo+2:wght@400;700;800;900&family=Ubuntu:wght@400;700&family=Source+Code+Pro:wght@400;700;900&family=Press+Start+2P&display=swap";
    document.head.appendChild(l);
  }
  if (!document.getElementById("fa-css")) {
    var l2 = document.createElement("link");
    l2.id = "fa-css"; l2.rel = "stylesheet";
    l2.href = "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/css/all.min.css";
    document.head.appendChild(l2);
  }
  // Load FA woff2 as FontFace for direct Unicode rendering
  var cdns = [
    "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.1/webfonts",
    "https://cdn.jsdelivr.net/npm/@fortawesome/fontawesome-free@6.5.1/webfonts",
  ];
  function tryLoadFA(family, file, weight) {
    var i = 0;
    function attempt() {
      if (i >= cdns.length) return;
      try {
        var f = new FontFace(family, "url(" + cdns[i] + "/" + file + ")", {weight:weight, style:"normal"});
        f.load().then(function(loaded) { document.fonts.add(loaded); }).catch(function() { i++; attempt(); });
      } catch(e) { i++; attempt(); }
    }
    attempt();
  }
  tryLoadFA("FA6Solid", "fa-solid-900.woff2", "900");
  tryLoadFA("FA6Brands", "fa-brands-400.woff2", "400");
}

var STYLES = [
  {id:"rects",name:"Overlapping Rects"},{id:"circles",name:"Circles & Arcs"},
  {id:"mixed",name:"Mixed Shapes"},{id:"diagonal",name:"Diagonal Bands"},
  {id:"blocks",name:"Grid Blocks"},{id:"radial",name:"Radial Arcs"},
];
var DISPLAYS = [
  {w:320,h:240,label:"QVGA 320x240"},
  {w:480,h:320,label:"HMI 480x320"},{w:800,h:480,label:"7in 800x480"},
  {w:1024,h:600,label:"10.1in 1024x600"},
];
var TITLE_POSITIONS = [
  {id:"center",name:"Center"},{id:"center-left",name:"Center Left"},
  {id:"top-left",name:"Top Left"},{id:"top-center",name:"Top Center"},
  {id:"bottom-center",name:"Bottom Center"},{id:"bottom-left",name:"Bottom Left"},
];
var ICON_POSITIONS = [
  {id:"left-of-title",name:"Left of Title"},{id:"above-title",name:"Above Title"},
  {id:"top-left",name:"Top Left"},{id:"top-right",name:"Top Right"},
  {id:"bottom-right",name:"Bottom Right"},{id:"center",name:"Center"},
];

function pick(rng,arr) { return arr[Math.floor(rng()*arr.length)]; }
function hexToRgba(hex,a) { var r=parseInt(hex.slice(1,3),16),g=parseInt(hex.slice(3,5),16),b=parseInt(hex.slice(5,7),16); return "rgba("+r+","+g+","+b+","+a+")"; }
function hexToLvgl(hex) { return "0x"+hex.slice(1).toUpperCase(); }

function generateShapes(rng,w,h,colors,style,count) {
  var shapes=[],minD=Math.min(w,h);
  function addRect(){var sw=Math.floor(rng()*w*0.5+minD*0.08),sh=Math.floor(rng()*h*0.5+minD*0.08),sx=Math.floor(rng()*w*1.3-w*0.15),sy=Math.floor(rng()*h*1.3-h*0.15),rot=Math.floor((rng()-0.5)*600),opa=Math.floor(rng()*120+30),rad=rng()>0.5?Math.floor(rng()*minD*0.08+2):0,c1=pick(rng,colors),c2=rng()>0.4?pick(rng,colors):c1,gd=rng()>0.5?"VER":"HOR";shapes.push({type:"rect",x:sx,y:sy,w:sw,h:sh,rotation:rot,opa:opa,radius:rad,c1:c1,c2:c2,gradDir:gd});}
  function addArc(){var cx=Math.floor(rng()*w),cy=Math.floor(rng()*h),r=Math.floor(rng()*minD*0.3+minD*0.05),th=Math.floor(rng()*r*0.6+3),sa=Math.floor(rng()*360),ea=sa+Math.floor(rng()*300+60),opa=Math.floor(rng()*140+30),c=pick(rng,colors),fi=rng()>0.6;shapes.push({type:"arc",cx:cx,cy:cy,r:r,thickness:th,startAngle:sa,endAngle:ea,opa:opa,c:c,filled:fi});}
  function addLine(){var x1=Math.floor(rng()*w*1.2-w*0.1),y1=Math.floor(rng()*h*1.2-h*0.1),ang=rng()*Math.PI,len=Math.floor(rng()*Math.max(w,h)*0.8+40),x2=Math.floor(x1+Math.cos(ang)*len),y2=Math.floor(y1+Math.sin(ang)*len),th=Math.floor(rng()*minD*0.06+2),opa=Math.floor(rng()*130+40),c=pick(rng,colors),rd=rng()>0.5;shapes.push({type:"line",x1:x1,y1:y1,x2:x2,y2:y2,thickness:th,opa:opa,c:c,rounded:rd});}
  for(var i=0;i<count;i++){switch(style){case"rects":addRect();break;case"circles":addArc();break;case"diagonal":rng()>0.3?addLine():addRect();break;case"blocks":addRect();break;case"radial":addArc();break;default:var r=rng();if(r<0.4)addRect();else if(r<0.7)addArc();else addLine();}}
  return shapes;
}

function renderPreview(canvas,cfg) {
  var w=cfg.width,h=cfg.height,pal=PALETTES[cfg.palette],ctx=canvas.getContext("2d");
  canvas.width=w;canvas.height=h;
  var rng=mkRng(cfg.seed),minD=Math.min(w,h);
  var bgA=rng()*Math.PI*0.5,grd=ctx.createLinearGradient(0,0,Math.cos(bgA)*w,Math.sin(bgA)*h+h*0.5);
  grd.addColorStop(0,pal.colors[0]);grd.addColorStop(1,pal.colors[1]);ctx.fillStyle=grd;ctx.fillRect(0,0,w,h);
  var count=Math.floor(cfg.complexity*1.5)+3;
  var shapes=generateShapes(mkRng(cfg.seed+100),w,h,pal.colors,cfg.style,count);
  for(var i=0;i<shapes.length;i++){var s=shapes[i];ctx.save();var alpha=s.opa/255;
    if(s.type==="rect"){ctx.globalAlpha=alpha;ctx.translate(s.x+s.w/2,s.y+s.h/2);ctx.rotate((s.rotation/10)*Math.PI/180);var rG=s.gradDir==="VER"?ctx.createLinearGradient(0,-s.h/2,0,s.h/2):ctx.createLinearGradient(-s.w/2,0,s.w/2,0);rG.addColorStop(0,s.c1);rG.addColorStop(1,s.c2);ctx.fillStyle=rG;ctx.beginPath();ctx.roundRect(-s.w/2,-s.h/2,s.w,s.h,s.radius);ctx.fill();}
    else if(s.type==="arc"){ctx.globalAlpha=alpha;if(s.filled){ctx.beginPath();ctx.arc(s.cx,s.cy,s.r,0,Math.PI*2);ctx.fillStyle=hexToRgba(s.c,alpha);ctx.fill();}else{ctx.beginPath();ctx.arc(s.cx,s.cy,s.r,s.startAngle*Math.PI/180,s.endAngle*Math.PI/180);ctx.strokeStyle=s.c;ctx.lineWidth=s.thickness;ctx.lineCap="round";ctx.stroke();}}
    else if(s.type==="line"){ctx.globalAlpha=alpha;ctx.beginPath();ctx.moveTo(s.x1,s.y1);ctx.lineTo(s.x2,s.y2);ctx.strokeStyle=s.c;ctx.lineWidth=s.thickness;ctx.lineCap=s.rounded?"round":"butt";ctx.stroke();}
    ctx.restore();}
  var tFont=FONTS[cfg.titleFont]||FONTS.montserrat;
  var sFont=FONTS[cfg.subtitleFont]||FONTS.montserrat;
  var wt=(FONT_WEIGHTS.find(function(fw){return fw.id===cfg.titleWeight;})||FONT_WEIGHTS[2]);
  var posMap={"center":{x:w/2,y:h*0.42,align:"center"},"center-left":{x:w*0.05,y:h*0.42,align:"left"},"top-left":{x:w*0.05,y:h*0.15,align:"left"},"top-center":{x:w/2,y:h*0.13,align:"center"},"bottom-center":{x:w/2,y:h*0.78,align:"center"},"bottom-left":{x:w*0.05,y:h*0.82,align:"left"}};
  var tp=posMap[cfg.titlePos]||posMap["center"];
  if(cfg.title){ctx.save();ctx.font=wt.css+" "+cfg.titleFontSize+"px "+tFont.preview;ctx.textAlign=tp.align;ctx.textBaseline="middle";ctx.shadowColor=pal.colors[0]+"CC";ctx.shadowBlur=cfg.titleFontSize*0.6;ctx.shadowOffsetX=2;ctx.shadowOffsetY=2;ctx.fillStyle=pal.text;ctx.fillText(cfg.title,tp.x,tp.y,w*0.92);ctx.restore();}
  if(cfg.subtitle){ctx.save();ctx.font="400 "+cfg.subtitleFontSize+"px "+sFont.preview;ctx.textAlign=tp.align;ctx.textBaseline="middle";ctx.fillStyle=hexToRgba(pal.text,0.7);ctx.fillText(cfg.subtitle,tp.x,tp.y+cfg.titleFontSize*0.7,w*0.85);ctx.restore();}
  if(cfg.version){var vSz=Math.max(9,Math.floor(minD*0.025));ctx.save();ctx.font="400 "+vSz+"px 'Segoe UI',sans-serif";ctx.textAlign="right";ctx.textBaseline="bottom";ctx.fillStyle=hexToRgba(pal.text,0.4);ctx.fillText(cfg.version,w-10,h-6);ctx.restore();}
  if(cfg.showProgress){var bW=w*0.45,bH=Math.max(3,Math.floor(minD*0.012)),bX=(w-bW)/2,bY=h*0.9;ctx.save();ctx.fillStyle=hexToRgba(pal.text,0.1);ctx.beginPath();ctx.roundRect(bX,bY,bW,bH,bH);ctx.fill();ctx.fillStyle=pal.accent;ctx.beginPath();ctx.roundRect(bX,bY,bW*(cfg.progressPct/100),bH,bH);ctx.fill();ctx.restore();}
}

// LVGL C code builder — uses array join to avoid template literal parsing issues
function generateLvglCode(cfg) {
  var w=cfg.width,h=cfg.height,pal=PALETTES[cfg.palette],minD=Math.min(w,h);
  var v9 = cfg.lvglVersion === "9";
  var count=Math.floor(cfg.complexity*1.5)+3;
  var shapes=generateShapes(mkRng(cfg.seed+100),w,h,pal.colors,cfg.style,count);
  var tFD=FONTS[cfg.titleFont]||FONTS.montserrat;
  var sFD=FONTS[cfg.subtitleFont]||FONTS.montserrat;
  function near(fd,t){return fd.sizes.reduce(function(p,c){return Math.abs(c-t)<Math.abs(p-t)?c:p;});}
  var aTS=near(tFD,cfg.titleFontSize),aSS=near(sFD,cfg.subtitleFontSize);
  var cn=cfg.customFontName||"my_custom_font";
  var tRef=cfg.titleFont==="custom"?tFD.lvgl(aTS,cn):tFD.lvgl(aTS);
  var sRef=cfg.subtitleFont==="custom"?sFD.lvgl(aSS,cn):sFD.lvgl(aSS);
  var tAlign=cfg.titlePos.includes("left")?"LEFT":"CENTER";
  var alignMap={"center":"LV_ALIGN_CENTER, 0, -(SPLASH_H*8/100)","center-left":"LV_ALIGN_LEFT_MID, SPLASH_W*5/100, -(SPLASH_H*8/100)","top-left":"LV_ALIGN_TOP_LEFT, SPLASH_W*5/100, SPLASH_H*10/100","top-center":"LV_ALIGN_TOP_MID, 0, SPLASH_H*8/100","bottom-center":"LV_ALIGN_BOTTOM_MID, 0, -(SPLASH_H*18/100)","bottom-left":"LV_ALIGN_BOTTOM_LEFT, SPLASH_W*5/100, -(SPLASH_H*14/100)"};
  var tAlignCode=alignMap[cfg.titlePos]||alignMap["center"];
  var icons=(cfg.icons||[]).filter(function(ic){return parseUnicode(ic.code);}).map(function(ic){return Object.assign({},ic,{parsed:parseUnicode(ic.code)});});

  var L = [];
  L.push("/**");
  L.push(" * LVGL Splash Screen - Pure Native Objects");
  L.push(" * "+w+"x"+h+" | "+pal.name+" | "+cfg.style+" | Seed: "+cfg.seed);
  L.push(" * Shapes: "+shapes.length+" | Icons: "+icons.length);
  L.push(" * Title: "+tFD.name+" "+aTS+"px | Subtitle: "+sFD.name+" "+aSS+"px");
  L.push(" *");
  L.push(" * FONT SETUP:");
  L.push(" *   "+tFD.setup(aTS,cn));
  L.push(" *   "+sFD.setup(aSS,cn));
  icons.forEach(function(ic){var sz=near({sizes:LVGL_SIZES},ic.size);var ff=ic.set==="brands"?"fa-brands-400":"fa-solid-900";L.push(" *   lv_font_conv --font "+ff+".ttf -o font_"+ff.replace("-","_")+"_"+sz+".c --size "+sz+" --format lvgl --bpp 4 -r 0x"+ic.parsed.hex);});
  L.push(" *");
  L.push(" * LVGL "+(v9?"9.x":"8.x")+". Zero images.");
  L.push(" */");
  L.push("");
  L.push('#include "lvgl.h"');
  L.push("");
  L.push("#define SPLASH_W  "+w);
  L.push("#define SPLASH_H  "+h);
  L.push("");

  var isB=function(k){return k==="montserrat"||k==="unscii";};
  var decl=new Set();
  if(!isB(cfg.titleFont)){var d=tRef.replace("&","");if(!decl.has(d)){L.push("LV_FONT_DECLARE("+d+");");decl.add(d);}}
  if(!isB(cfg.subtitleFont)){var d2=sRef.replace("&","");if(!decl.has(d2)){L.push("LV_FONT_DECLARE("+d2+");");decl.add(d2);}}
  icons.forEach(function(ic){var sz=near({sizes:LVGL_SIZES},ic.size);var fn=ic.set==="brands"?"font_fa_brands_"+sz:"font_fa_solid_"+sz;if(!decl.has(fn)){L.push("LV_FONT_DECLARE("+fn+");");decl.add(fn);}});
  if(decl.size>0)L.push("");

  L.push("static lv_obj_t *splash_scr = NULL;");
  if(cfg.showProgress)L.push("static lv_obj_t *splash_bar = NULL;");
  L.push("");
  L.push("void splash_screen_create(void)");
  L.push("{");
  L.push("    splash_scr = lv_obj_create(NULL);");
  L.push("    lv_obj_set_size(splash_scr, SPLASH_W, SPLASH_H);");
  L.push("    "+( v9 ? "lv_obj_remove_flag" : "lv_obj_clear_flag")+"(splash_scr, LV_OBJ_FLAG_SCROLLABLE);");
  L.push("    lv_obj_set_style_bg_color(splash_scr, lv_color_hex("+hexToLvgl(pal.colors[0])+"), 0);");
  L.push("    lv_obj_set_style_bg_grad_color(splash_scr, lv_color_hex("+hexToLvgl(pal.colors[1])+"), 0);");
  L.push("    lv_obj_set_style_bg_grad_dir(splash_scr, LV_GRAD_DIR_VER, 0);");
  L.push("    lv_obj_set_style_bg_opa(splash_scr, LV_OPA_COVER, 0);");
  L.push("");

  shapes.forEach(function(s,i){
    if(s.type==="rect"){
      L.push("    { /* rect "+i+" */");
      L.push("        lv_obj_t *r = lv_obj_create(splash_scr);");
      L.push("        lv_obj_remove_style_all(r);");
      L.push("        lv_obj_set_size(r, "+s.w+", "+s.h+");");
      L.push("        lv_obj_set_pos(r, "+s.x+", "+s.y+");");
      L.push("        lv_obj_set_style_bg_color(r, lv_color_hex("+hexToLvgl(s.c1)+"), 0);");
      L.push("        lv_obj_set_style_bg_grad_color(r, lv_color_hex("+hexToLvgl(s.c2)+"), 0);");
      L.push("        lv_obj_set_style_bg_grad_dir(r, LV_GRAD_DIR_"+s.gradDir+", 0);");
      L.push("        lv_obj_set_style_bg_opa(r, "+s.opa+", 0);");
      L.push("        lv_obj_set_style_radius(r, "+s.radius+", 0);");
      L.push("        lv_obj_set_style_border_width(r, 0, 0);");
      L.push("        "+(v9?"lv_obj_remove_flag":"lv_obj_clear_flag")+"(r, LV_OBJ_FLAG_SCROLLABLE);");
      if(Math.abs(s.rotation)>10){L.push("        lv_obj_set_style_transform_angle(r, "+s.rotation+", 0);");L.push("        lv_obj_set_style_transform_pivot_x(r, "+Math.floor(s.w/2)+", 0);");L.push("        lv_obj_set_style_transform_pivot_y(r, "+Math.floor(s.h/2)+", 0);");}
      L.push("    }");L.push("");
    } else if(s.type==="arc"){
      if(s.filled){
        L.push("    { /* circle "+i+" */");
        L.push("        lv_obj_t *c = lv_obj_create(splash_scr);");
        L.push("        lv_obj_remove_style_all(c);");
        L.push("        lv_obj_set_size(c, "+s.r*2+", "+s.r*2+");");
        L.push("        lv_obj_set_pos(c, "+(s.cx-s.r)+", "+(s.cy-s.r)+");");
        L.push("        lv_obj_set_style_bg_color(c, lv_color_hex("+hexToLvgl(s.c)+"), 0);");
        L.push("        lv_obj_set_style_bg_opa(c, "+s.opa+", 0);");
        L.push("        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);");
        L.push("        lv_obj_set_style_border_width(c, 0, 0);");
        L.push("        "+(v9?"lv_obj_remove_flag":"lv_obj_clear_flag")+"(c, LV_OBJ_FLAG_SCROLLABLE);");
        L.push("    }");L.push("");
      } else {
        var sN=((s.startAngle%360)+360)%360,eN=sN+((s.endAngle-s.startAngle)%360+360)%360;
        L.push("    { /* arc "+i+" */");
        L.push("        lv_obj_t *a = lv_arc_create(splash_scr);");
        L.push("        lv_obj_set_size(a, "+s.r*2+", "+s.r*2+");");
        L.push("        lv_obj_set_pos(a, "+(s.cx-s.r)+", "+(s.cy-s.r)+");");
        if(v9){L.push("        lv_arc_set_bg_start_angle(a, "+sN+");");L.push("        lv_arc_set_bg_end_angle(a, "+Math.min(eN,sN+350)+");");}
        else{L.push("        lv_arc_set_bg_angles(a, "+sN+", "+Math.min(eN,sN+350)+");");}
        L.push("        lv_arc_set_value(a, 0);");
        L.push("        lv_obj_set_style_arc_width(a, "+s.thickness+", LV_PART_MAIN);");
        L.push("        lv_obj_set_style_arc_color(a, lv_color_hex("+hexToLvgl(s.c)+"), LV_PART_MAIN);");
        L.push("        lv_obj_set_style_arc_opa(a, "+s.opa+", LV_PART_MAIN);");
        L.push("        lv_obj_set_style_arc_width(a, 0, LV_PART_INDICATOR);");
        L.push("        "+(v9?"lv_obj_remove_flag":"lv_obj_clear_flag")+"(a, LV_OBJ_FLAG_CLICKABLE);");
        L.push("        lv_obj_remove_style(a, NULL, LV_PART_KNOB);");
        L.push("    }");L.push("");
      }
    } else if(s.type==="line"){
      var ptType = v9 ? "lv_point_precise_t" : "lv_point_t";
      L.push("    { /* line "+i+" */");
      L.push("        static "+ptType+" pts_"+i+"[] = { {"+s.x1+","+s.y1+"}, {"+s.x2+","+s.y2+"} };");
      L.push("        lv_obj_t *l = lv_line_create(splash_scr);");
      L.push("        lv_line_set_points(l, pts_"+i+", 2);");
      L.push("        lv_obj_set_style_line_color(l, lv_color_hex("+hexToLvgl(s.c)+"), 0);");
      L.push("        lv_obj_set_style_line_width(l, "+s.thickness+", 0);");
      L.push("        lv_obj_set_style_line_opa(l, "+s.opa+", 0);");
      L.push("        lv_obj_set_style_line_rounded(l, "+(s.rounded?"true":"false")+", 0);");
      L.push("    }");L.push("");
    }
  });

  icons.forEach(function(ic,i){
    var sz=near({sizes:LVGL_SIZES},ic.size);
    var fn=ic.set==="brands"?"font_fa_brands_"+sz:"font_fa_solid_"+sz;
    var col=ic.color==="accent"?"lv_color_hex("+hexToLvgl(pal.accent)+")":ic.color==="text"?"lv_color_hex("+hexToLvgl(pal.text)+")":"lv_color_hex("+hexToLvgl(ic.color)+")";
    var px=Math.round(ic.x/100*w),py=Math.round(ic.y/100*h);
    L.push("    { /* icon "+i+": "+ic.set+" U+"+ic.parsed.hex+" */");
    L.push("        lv_obj_t *ico = lv_label_create(splash_scr);");
    L.push('        lv_label_set_text(ico, "'+ic.parsed.escape+'");');
    L.push("        lv_obj_set_style_text_font(ico, &"+fn+", 0);");
    L.push("        lv_obj_set_style_text_color(ico, "+col+", 0);");
    L.push("        lv_obj_set_style_text_opa(ico, "+Math.round(ic.opacity*255)+", 0);");
    L.push("        lv_obj_set_pos(ico, "+px+", "+py+");");
    L.push("    }");L.push("");
  });

  if(cfg.title){
    L.push("    /* Title */");
    L.push("    lv_obj_t *lbl_title = lv_label_create(splash_scr);");
    L.push('    lv_label_set_text(lbl_title, "'+cfg.title+'");');
    L.push("    lv_obj_set_style_text_font(lbl_title, "+tRef+", 0);");
    L.push("    lv_obj_set_style_text_color(lbl_title, lv_color_hex("+hexToLvgl(pal.text)+"), 0);");
    L.push("    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_"+tAlign+", 0);");
    L.push("    lv_obj_align(lbl_title, "+tAlignCode+");");
    L.push("    lv_obj_set_style_shadow_color(lbl_title, lv_color_hex("+hexToLvgl(pal.colors[0])+"), 0);");
    L.push("    lv_obj_set_style_shadow_width(lbl_title, "+Math.floor(cfg.titleFontSize*0.4)+", 0);");
    L.push("    lv_obj_set_style_shadow_opa(lbl_title, 180, 0);");
    L.push("");
  }
  if(cfg.subtitle){
    L.push("    lv_obj_t *lbl_sub = lv_label_create(splash_scr);");
    L.push('    lv_label_set_text(lbl_sub, "'+cfg.subtitle+'");');
    L.push("    lv_obj_set_style_text_font(lbl_sub, "+sRef+", 0);");
    L.push("    lv_obj_set_style_text_color(lbl_sub, lv_color_hex("+hexToLvgl(pal.text)+"), 0);");
    L.push("    lv_obj_set_style_text_opa(lbl_sub, 180, 0);");
    L.push("    lv_obj_align_to(lbl_sub, lbl_title, LV_ALIGN_OUT_BOTTOM_"+(cfg.titlePos.includes("left")?"LEFT":"MID")+", 0, 8);");
    L.push("");
  }
  if(cfg.version){
    L.push("    lv_obj_t *lbl_ver = lv_label_create(splash_scr);");
    L.push('    lv_label_set_text(lbl_ver, "'+cfg.version+'");');
    L.push("    lv_obj_set_style_text_font(lbl_ver, &lv_font_montserrat_12, 0);");
    L.push("    lv_obj_set_style_text_color(lbl_ver, lv_color_hex("+hexToLvgl(pal.text)+"), 0);");
    L.push("    lv_obj_set_style_text_opa(lbl_ver, 100, 0);");
    L.push("    lv_obj_align(lbl_ver, LV_ALIGN_BOTTOM_RIGHT, -10, -6);");
    L.push("");
  }
  if(cfg.showProgress){
    var barH=Math.max(4,Math.floor(minD*0.013));
    L.push("    splash_bar = lv_bar_create(splash_scr);");
    L.push("    lv_obj_set_size(splash_bar, SPLASH_W*45/100, "+barH+");");
    L.push("    lv_obj_align(splash_bar, LV_ALIGN_BOTTOM_MID, 0, -(SPLASH_H*8/100));");
    L.push("    lv_obj_set_style_bg_color(splash_bar, lv_color_hex("+hexToLvgl(pal.text)+"), LV_PART_MAIN);");
    L.push("    lv_obj_set_style_bg_opa(splash_bar, 25, LV_PART_MAIN);");
    L.push("    lv_obj_set_style_bg_color(splash_bar, lv_color_hex("+hexToLvgl(pal.accent)+"), LV_PART_INDICATOR);");
    L.push("    lv_obj_set_style_radius(splash_bar, LV_RADIUS_CIRCLE, 0);");
    L.push("    lv_obj_set_style_radius(splash_bar, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);");
    L.push("    lv_bar_set_value(splash_bar, 0, LV_ANIM_OFF);");
    L.push("");
  }
  L.push("    "+(v9?"lv_screen_load":"lv_scr_load")+"(splash_scr);");
  L.push("}");
  if(cfg.showProgress){L.push("");L.push("void splash_set_progress(int32_t val)");L.push("{");L.push("    if (splash_bar) lv_bar_set_value(splash_bar, val, LV_ANIM_ON);");L.push("}");}
  L.push("");L.push("void splash_screen_delete(void)");L.push("{");L.push("    if (splash_scr) {");L.push("        "+(v9?"lv_obj_delete":"lv_obj_del")+"(splash_scr);");L.push("        splash_scr = NULL;");
  if(cfg.showProgress)L.push("        splash_bar = NULL;");
  L.push("    }");L.push("}");
  return L.join("\n");
}

// Draggable icon overlay
function DraggableIcon(props) {
  var icon=props.icon, scale=props.scale, onUpdate=props.onUpdate, onRemove=props.onRemove, selected=props.selected, onSelect=props.onSelect;
  var ref = useRef(null);
  var dragRef = useRef({dragging:false,resizing:false,sx:0,sy:0,ix:0,iy:0,isz:0});
  var parsed = parseUnicode(icon.code);
  if (!parsed) return null;

  var handleMouseDown = function(e) {
    e.stopPropagation(); onSelect();
    dragRef.current = {dragging:true,resizing:false,sx:e.clientX,sy:e.clientY,ix:icon.x,iy:icon.y,isz:icon.size};
    var parentRect = ref.current.parentElement.getBoundingClientRect();
    var onMove = function(ev) {
      if (!dragRef.current.dragging) return;
      var dx = (ev.clientX-dragRef.current.sx)/scale;
      var dy = (ev.clientY-dragRef.current.sy)/scale;
      var pw = parentRect.width/scale, ph = parentRect.height/scale;
      onUpdate({x:Math.max(0,Math.min(100,dragRef.current.ix+dx/pw*100)),y:Math.max(0,Math.min(100,dragRef.current.iy+dy/ph*100))});
    };
    var onUp = function() {dragRef.current.dragging=false;window.removeEventListener("mousemove",onMove);window.removeEventListener("mouseup",onUp);};
    window.addEventListener("mousemove",onMove);window.addEventListener("mouseup",onUp);
  };

  var handleResizeDown = function(e) {
    e.stopPropagation(); e.preventDefault();
    dragRef.current = {dragging:false,resizing:true,sx:e.clientX,sy:e.clientY,ix:icon.x,iy:icon.y,isz:icon.size};
    var onMove = function(ev) {
      if (!dragRef.current.resizing) return;
      var dy = (ev.clientY-dragRef.current.sy)/scale;
      onUpdate({size:Math.max(12,Math.min(120,dragRef.current.isz+dy*0.5))});
    };
    var onUp = function() {dragRef.current.resizing=false;window.removeEventListener("mousemove",onMove);window.removeEventListener("mouseup",onUp);};
    window.addEventListener("mousemove",onMove);window.addEventListener("mouseup",onUp);
  };

  var color = icon.color==="accent"?props.accentColor:icon.color==="text"?props.textColor:icon.color;
  var faFamily = icon.set==="brands"?"FA6Brands, 'Font Awesome 6 Brands'":"FA6Solid, 'Font Awesome 6 Free'";

  return (
    <div ref={ref} onMouseDown={handleMouseDown} onClick={function(e){e.stopPropagation();}} style={{position:"absolute",left:icon.x+"%",top:icon.y+"%",transform:"translate(-50%,-50%)",cursor:"grab",userSelect:"none",lineHeight:1,outline:selected?"2px solid #fff":"none",outlineOffset:4,borderRadius:4}}>
      <span style={{fontFamily:faFamily,fontWeight:icon.set==="brands"?400:900,fontSize:icon.size*scale,color:color,opacity:icon.opacity,filter:"drop-shadow(0 0 "+4*scale+"px rgba(0,0,0,0.5))",display:"block"}}>{parsed.char}</span>
      {selected && (
        <div>
          <div onMouseDown={handleResizeDown} style={{position:"absolute",right:-6,bottom:-6,width:10,height:10,background:"#fff",borderRadius:2,cursor:"nwse-resize",border:"1px solid #333"}} />
          <div onClick={function(e){e.stopPropagation();onRemove();}} style={{position:"absolute",right:-8,top:-8,width:16,height:16,background:"#e83f6f",borderRadius:8,cursor:"pointer",display:"flex",alignItems:"center",justifyContent:"center",fontSize:10,color:"#fff",fontWeight:900,lineHeight:1}}>x</div>
        </div>
      )}
    </div>
  );
}

// Main component
export default function LvglSplashV6() {
  var canvasRef = useRef(null);
  var [cfg,setCfg] = useState({
    width:800,height:480,palette:"cyberpunk",style:"mixed",
    title:"FILMACHINE",subtitle:"Digital Cinema Engine",version:"v2.1.0",
    complexity:10,showProgress:true,progressPct:65,
    titlePos:"center-left",titleFont:"montserrat",titleFontSize:48,
    subtitleFont:"montserrat",subtitleFontSize:20,titleWeight:"extrabold",
    customFontName:"my_custom_font",icons:[],seed:7742,
    lvglVersion:"9",
  });
  var [tab,setTab] = useState("preview");
  var [copied,setCopied] = useState(false);
  var [selIcon,setSelIcon] = useState(null);
  var [newIconCode,setNewIconCode] = useState("");
  var [newIconSet,setNewIconSet] = useState("solid");

  var set = useCallback(function(k,v){setCfg(function(c){var n={};Object.assign(n,c);n[k]=v;return n;});},[]);
  var pal = PALETTES[cfg.palette];
  var scale = Math.min(1,620/cfg.width,420/cfg.height);
  var shapeCount = Math.floor(cfg.complexity*1.5)+3;

  useEffect(function(){if(canvasRef.current)renderPreview(canvasRef.current,cfg);},[cfg,tab]);

  var addIcon = function() {
    var p = parseUnicode(newIconCode); if(!p) return;
    var ic = {id:Date.now(),code:newIconCode.replace(/[^0-9a-fA-F]/g,""),set:newIconSet,size:48,x:50,y:30,color:"accent",opacity:0.85};
    set("icons",cfg.icons.concat([ic])); setNewIconCode(""); setSelIcon(ic.id);
  };
  var updateIcon = function(id,patch) { set("icons",cfg.icons.map(function(ic){return ic.id===id?Object.assign({},ic,patch):ic;})); };
  var removeIcon = function(id) { set("icons",cfg.icons.filter(function(ic){return ic.id!==id;})); if(selIcon===id)setSelIcon(null); };
  var selIconObj = cfg.icons.find(function(ic){return ic.id===selIcon;});

  var randomize = function() {
    var pk=Object.keys(PALETTES),sk=STYLES.map(function(s){return s.id;}),tp=TITLE_POSITIONS.map(function(t){return t.id;}),fk=Object.keys(FONTS).filter(function(k){return k!=="custom";});
    setCfg(function(c){return Object.assign({},c,{seed:Math.floor(Math.random()*99999),palette:pk[Math.floor(Math.random()*pk.length)],style:sk[Math.floor(Math.random()*sk.length)],titlePos:tp[Math.floor(Math.random()*tp.length)],titleFont:fk[Math.floor(Math.random()*fk.length)],complexity:Math.floor(Math.random()*18)+4});});
  };
  var copyCode = function(){navigator.clipboard.writeText(generateLvglCode(cfg));setCopied(true);setTimeout(function(){setCopied(false);},2000);};
  var downloadC = function(){var b=new Blob([generateLvglCode(cfg)],{type:"text/plain"});var a=document.createElement("a");a.download="splash_screen.c";a.href=URL.createObjectURL(b);a.click();};
  var downloadH = function(){var hc="#ifndef SPLASH_SCREEN_H\n#define SPLASH_SCREEN_H\n\n#include \"lvgl.h\"\n\nvoid splash_screen_create(void);\n"+(cfg.showProgress?"void splash_set_progress(int32_t val);\n":"")+"void splash_screen_delete(void);\n\n#endif\n";var b=new Blob([hc],{type:"text/plain"});var a=document.createElement("a");a.download="splash_screen.h";a.href=URL.createObjectURL(b);a.click();};

  var parsedNew = parseUnicode(newIconCode);
  var inp = {width:"100%",boxSizing:"border-box",padding:"5px 8px",background:"#0f0f14",border:"1px solid #28282f",borderRadius:4,color:"#ccc",fontSize:11,outline:"none",fontFamily:"inherit"};
  var sel = Object.assign({},inp,{cursor:"pointer"});
  var chip = function(on,ac){return{padding:"4px 9px",borderRadius:4,cursor:"pointer",fontSize:10,fontWeight:600,border:"1.5px solid "+(on?ac:"#28282f"),background:on?ac+"20":"transparent",color:on?ac:"#777",fontFamily:"inherit",transition:"all .12s"};};

  var Lbl = function(p){return <div style={{fontSize:9,fontWeight:700,textTransform:"uppercase",color:"#4a4a55",letterSpacing:1.2,margin:"12px 0 4px"}}>{p.children}</div>;};
  var SubLbl = function(p){return <div style={{fontSize:8,color:"#444",marginBottom:2}}>{p.children}</div>;};
  var fontGroups = {};
  Object.entries(FONTS).forEach(function(e){var k=e[0],f=e[1];if(!fontGroups[f.group])fontGroups[f.group]=[];fontGroups[f.group].push([k,f]);});
  var faFamily = function(s){return s==="brands"?"FA6Brands, 'Font Awesome 6 Brands'":"FA6Solid, 'Font Awesome 6 Free'";};

  return (
    <div style={{minHeight:"100vh",background:"#08080c",color:"#d4d4d8",fontFamily:"'SF Mono','Fira Code','Consolas',monospace",fontSize:11}}>
      <div style={{padding:"12px 18px",borderBottom:"1px solid #1a1a22",display:"flex",alignItems:"center",gap:10,background:"#0a0a10"}}>
        <div style={{width:28,height:28,borderRadius:5,background:"linear-gradient(135deg,"+pal.accent+","+(pal.colors[3]||pal.colors[1])+")",display:"flex",alignItems:"center",justifyContent:"center",fontSize:13,fontWeight:900,color:"#fff"}}>S</div>
        <div style={{flex:1}}>
          <div style={{fontSize:13,fontWeight:700}}>LVGL Splash Gen <span style={{color:"#444",fontWeight:400,fontSize:10}}>v6</span></div>
          <div style={{fontSize:9,color:"#444"}}>Custom fonts - Draggable FA icons - Pure LVGL - 0 images</div>
        </div>
        <button onClick={randomize} style={{padding:"5px 12px",borderRadius:5,border:"1px solid "+pal.accent+"44",background:pal.accent+"15",color:pal.accent,fontWeight:700,fontSize:10,cursor:"pointer",fontFamily:"inherit"}}>Randomize</button>
      </div>

      <div style={{display:"flex",flexWrap:"wrap"}}>
        <div style={{width:300,padding:"8px 14px",borderRight:"1px solid #1a1a22",overflowY:"auto",maxHeight:"calc(100vh - 56px)"}}>
          <Lbl>LVGL Version</Lbl>
          <div style={{display:"flex",gap:3}}>
            {["8","9"].map(function(v){return <button key={v} onClick={function(){set("lvglVersion",v);}} style={Object.assign({},chip(cfg.lvglVersion===v,pal.accent),{flex:1,padding:"5px 0",fontSize:12})}>{"v"+v+".x"}</button>;})}
          </div>

          <Lbl>Display</Lbl>
          <select value={cfg.width+"x"+cfg.height} onChange={function(e){var p=e.target.value.split("x").map(Number);set("width",p[0]);set("height",p[1]);}} style={sel}>
            {DISPLAYS.map(function(d){return <option key={d.label} value={d.w+"x"+d.h}>{d.label}</option>;})}
          </select>
          <div style={{display:"flex",gap:5,marginTop:4}}>
            <input type="number" value={cfg.width} onChange={function(e){set("width",+e.target.value);}} style={Object.assign({},inp,{flex:1})} min={100} max={2048} />
            <span style={{color:"#333",lineHeight:"28px"}}>x</span>
            <input type="number" value={cfg.height} onChange={function(e){set("height",+e.target.value);}} style={Object.assign({},inp,{flex:1})} min={100} max={2048} />
          </div>

          <Lbl>Palette</Lbl>
          <div style={{display:"grid",gridTemplateColumns:"1fr 1fr",gap:3}}>
            {Object.entries(PALETTES).map(function(e){var k=e[0],p=e[1];return(
              <button key={k} onClick={function(){set("palette",k);}} style={Object.assign({},chip(cfg.palette===k,p.accent),{display:"flex",alignItems:"center",gap:4,padding:"3px 6px"})}>
                <div style={{display:"flex",gap:1}}>{p.colors.slice(0,4).map(function(c,i){return <div key={i} style={{width:7,height:7,borderRadius:1.5,background:c}} />;})}</div>
                <span style={{fontSize:8.5,overflow:"hidden",textOverflow:"ellipsis",whiteSpace:"nowrap"}}>{p.name}</span>
              </button>);})}
          </div>

          <Lbl>Shape Style</Lbl>
          <div style={{display:"grid",gridTemplateColumns:"1fr 1fr",gap:3}}>
            {STYLES.map(function(s){return <button key={s.id} onClick={function(){set("style",s.id);}} style={chip(cfg.style===s.id,pal.accent)}>{s.name}</button>;})}
          </div>

          <Lbl>Complexity ({cfg.complexity})</Lbl>
          <input type="range" min={2} max={30} value={cfg.complexity} onChange={function(e){set("complexity",+e.target.value);}} style={{width:"100%",accentColor:pal.accent}} />

          <Lbl>Title</Lbl><input value={cfg.title} onChange={function(e){set("title",e.target.value);}} style={inp} />
          <Lbl>Subtitle</Lbl><input value={cfg.subtitle} onChange={function(e){set("subtitle",e.target.value);}} style={inp} />
          <Lbl>Version</Lbl><input value={cfg.version} onChange={function(e){set("version",e.target.value);}} style={inp} />
          <Lbl>Title Position</Lbl>
          <select value={cfg.titlePos} onChange={function(e){set("titlePos",e.target.value);}} style={sel}>
            {TITLE_POSITIONS.map(function(p){return <option key={p.id} value={p.id}>{p.name}</option>;})}
          </select>

          <Lbl>Title Font</Lbl>
          <select value={cfg.titleFont} onChange={function(e){set("titleFont",e.target.value);}} style={sel}>
            {Object.entries(fontGroups).map(function(e){var g=e[0],fs=e[1];return <optgroup key={g} label={g}>{fs.map(function(f){return <option key={f[0]} value={f[0]}>{f[1].name}</option>;})}</optgroup>;})}
          </select>
          <div style={{display:"flex",gap:4,marginTop:4}}>
            <div style={{flex:1}}><SubLbl>Size</SubLbl><select value={cfg.titleFontSize} onChange={function(e){set("titleFontSize",+e.target.value);}} style={sel}>{(FONTS[cfg.titleFont]||FONTS.montserrat).sizes.map(function(s){return <option key={s} value={s}>{s}px</option>;})}</select></div>
            <div style={{flex:1}}><SubLbl>Weight</SubLbl><select value={cfg.titleWeight} onChange={function(e){set("titleWeight",e.target.value);}} style={sel}>{FONT_WEIGHTS.map(function(w){return <option key={w.id} value={w.id}>{w.name}</option>;})}</select></div>
          </div>

          <Lbl>Subtitle Font</Lbl>
          <div style={{display:"flex",gap:4}}>
            <div style={{flex:1}}><select value={cfg.subtitleFont} onChange={function(e){set("subtitleFont",e.target.value);}} style={sel}>{Object.entries(fontGroups).map(function(e){var g=e[0],fs=e[1];return <optgroup key={g} label={g}>{fs.map(function(f){return <option key={f[0]} value={f[0]}>{f[1].name}</option>;})}</optgroup>;})}</select></div>
            <div style={{flex:1}}><SubLbl>Size</SubLbl><select value={cfg.subtitleFontSize} onChange={function(e){set("subtitleFontSize",+e.target.value);}} style={sel}>{(FONTS[cfg.subtitleFont]||FONTS.montserrat).sizes.map(function(s){return <option key={s} value={s}>{s}px</option>;})}</select></div>
          </div>

          {(cfg.titleFont==="custom"||cfg.subtitleFont==="custom") && <div><Lbl>Custom Font Name</Lbl><input value={cfg.customFontName} onChange={function(e){set("customFontName",e.target.value);}} style={inp} placeholder="my_custom_font" /></div>}

          <Lbl>FontAwesome Icons ({cfg.icons.length})</Lbl>
          <div style={{fontSize:8,color:"#555",marginBottom:5,lineHeight:1.4}}>
            <a href="https://fontawesome.com/icons" target="_blank" rel="noopener" style={{color:pal.accent}}>fontawesome.com/icons</a> - copia Unicode - incolla qui - Add. Trascina nel preview.
          </div>
          <div style={{display:"flex",gap:3,alignItems:"center"}}>
            <span style={{color:"#555",fontSize:10}}>U+</span>
            <input value={newIconCode} onChange={function(e){setNewIconCode(e.target.value.replace(/[^0-9a-fA-F]/g,"").slice(0,5));}} onKeyDown={function(e){if(e.key==="Enter")addIcon();}} style={Object.assign({},inp,{flex:1,fontFamily:"monospace",letterSpacing:2})} placeholder="f722" maxLength={5} />
            <select value={newIconSet} onChange={function(e){setNewIconSet(e.target.value);}} style={Object.assign({},sel,{width:70,flex:"none"})}><option value="solid">Solid</option><option value="brands">Brands</option></select>
            <button onClick={addIcon} disabled={!parsedNew} style={Object.assign({},chip(!!parsedNew,pal.accent),{padding:"5px 10px",opacity:parsedNew?1:0.4})}>Add</button>
          </div>
          {parsedNew && <div style={{marginTop:4,display:"flex",alignItems:"center",gap:6}}><span style={{fontFamily:faFamily(newIconSet),fontWeight:newIconSet==="brands"?400:900,fontSize:22,color:pal.accent}}>{parsedNew.char}</span><span style={{fontSize:9,color:"#666"}}>Preview</span></div>}

          {cfg.icons.length > 0 && <div style={{marginTop:8,display:"flex",flexDirection:"column",gap:3}}>
            {cfg.icons.map(function(ic){var p=parseUnicode(ic.code);return p ? (
              <div key={ic.id} onClick={function(){setSelIcon(ic.id);}} style={{display:"flex",alignItems:"center",gap:6,padding:"4px 6px",background:selIcon===ic.id?pal.accent+"18":"#111116",border:"1px solid "+(selIcon===ic.id?pal.accent+"55":"#28282f"),borderRadius:4,cursor:"pointer"}}>
                <span style={{fontFamily:faFamily(ic.set),fontWeight:ic.set==="brands"?400:900,fontSize:16,color:pal.accent,width:20,textAlign:"center"}}>{p.char}</span>
                <span style={{fontSize:9,color:"#999",flex:1}}>U+{p.hex} - {ic.set} - {Math.round(ic.size)}px</span>
                <button onClick={function(e){e.stopPropagation();removeIcon(ic.id);}} style={{background:"none",border:"none",color:"#666",cursor:"pointer",fontSize:12,padding:"0 2px"}}>x</button>
              </div>
            ) : null;})}
          </div>}

          {selIconObj && <div style={{marginTop:8,padding:8,background:"#0d0d12",borderRadius:4,border:"1px solid "+pal.accent+"33"}}>
            <div style={{fontSize:9,color:pal.accent,marginBottom:6,fontWeight:700}}>Selected: U+{(parseUnicode(selIconObj.code)||{}).hex}</div>
            <div style={{display:"flex",gap:4}}>
              <div style={{flex:1}}><SubLbl>Size ({Math.round(selIconObj.size)})</SubLbl><input type="range" min={12} max={120} value={selIconObj.size} onChange={function(e){updateIcon(selIcon,{size:+e.target.value});}} style={{width:"100%",accentColor:pal.accent}} /></div>
              <div style={{flex:1}}><SubLbl>Opacity ({Math.round(selIconObj.opacity*100)}%)</SubLbl><input type="range" min={10} max={100} value={Math.round(selIconObj.opacity*100)} onChange={function(e){updateIcon(selIcon,{opacity:e.target.value/100});}} style={{width:"100%",accentColor:pal.accent}} /></div>
            </div>
            <SubLbl>Color</SubLbl>
            <div style={{display:"flex",gap:3}}>
              {["accent","text"].map(function(c){return <button key={c} onClick={function(){updateIcon(selIcon,{color:c});}} style={Object.assign({},chip(selIconObj.color===c,pal.accent),{fontSize:9,padding:"2px 8px"})}>{c}</button>;})}
              <input type="color" value={selIconObj.color.startsWith("#")?selIconObj.color:pal.accent} onChange={function(e){updateIcon(selIcon,{color:e.target.value});}} style={{width:24,height:22,border:"1px solid #28282f",borderRadius:3,cursor:"pointer"}} />
            </div>
            <SubLbl>Set</SubLbl>
            <div style={{display:"flex",gap:3}}>{["solid","brands"].map(function(s){return <button key={s} onClick={function(){updateIcon(selIcon,{set:s});}} style={Object.assign({},chip(selIconObj.set===s,pal.accent),{fontSize:9,padding:"2px 8px"})}>{s}</button>;})}</div>
          </div>}

          <Lbl>Progress Bar</Lbl>
          <label style={{display:"flex",alignItems:"center",gap:6,cursor:"pointer",fontSize:10,color:"#888"}}>
            <input type="checkbox" checked={cfg.showProgress} onChange={function(e){set("showProgress",e.target.checked);}} /> Mostra barra
          </label>
          {cfg.showProgress && <input type="range" min={0} max={100} value={cfg.progressPct} onChange={function(e){set("progressPct",+e.target.value);}} style={{width:"100%",accentColor:pal.accent,marginTop:4}} />}

          <Lbl>Seed</Lbl>
          <div style={{display:"flex",gap:4}}>
            <input type="number" value={cfg.seed} onChange={function(e){set("seed",+e.target.value);}} style={Object.assign({},inp,{flex:1})} />
            <button onClick={function(){set("seed",Math.floor(Math.random()*99999));}} style={Object.assign({},chip(false,pal.accent),{whiteSpace:"nowrap",padding:"4px 8px"})}>New</button>
          </div>
          <div style={{height:20}} />
        </div>

        <div style={{flex:1,minWidth:0,padding:"12px 18px"}}>
          <div style={{display:"flex",gap:3,marginBottom:10,flexWrap:"wrap"}}>
            {["preview","code"].map(function(t){return <button key={t} onClick={function(){setTab(t);}} style={Object.assign({},chip(tab===t,pal.accent),{padding:"5px 16px",fontSize:11})}>{t==="code"?"LVGL Code":"Preview"}</button>;})}
            <div style={{flex:1}} />
            {tab==="code" && <div style={{display:"flex",gap:3}}>
              <button onClick={downloadC} style={Object.assign({},chip(false,"#3b82f6"),{padding:"4px 10px",fontSize:10})}>Download .c</button>
              <button onClick={downloadH} style={Object.assign({},chip(false,"#3b82f6"),{padding:"4px 10px",fontSize:10})}>Download .h</button>
              <button onClick={copyCode} style={Object.assign({},chip(copied,"#22c55e"),{padding:"4px 10px",fontSize:10,color:copied?"#22c55e":"#777"})}>{copied?"Copied!":"Copy"}</button>
            </div>}
          </div>

          <div style={{background:"#060609",borderRadius:8,border:"1px solid #1a1a22",display:tab==="preview"?"flex":"none",alignItems:"center",justifyContent:"center",padding:16,minHeight:380}}>
            <div style={{position:"relative",border:"2px solid #1e1e28",borderRadius:4,overflow:"hidden",boxShadow:"0 0 50px "+pal.accent+"10"}} onClick={function(){setSelIcon(null);}}>
              <canvas ref={canvasRef} style={{width:cfg.width*scale,height:cfg.height*scale,display:"block"}} />
              {cfg.icons.map(function(ic){return (
                <DraggableIcon key={ic.id} icon={ic} scale={scale} selected={selIcon===ic.id}
                  accentColor={pal.accent} textColor={pal.text}
                  onSelect={function(){setSelIcon(ic.id);}}
                  onUpdate={function(patch){updateIcon(ic.id,patch);}}
                  onRemove={function(){removeIcon(ic.id);}} />
              );})}
            </div>
          </div>

          {tab==="code" && <div style={{background:"#060609",borderRadius:8,border:"1px solid #1a1a22"}}><pre style={{margin:0,padding:"14px 16px",fontSize:10,color:"#9a9aaa",overflowX:"auto",maxHeight:520,lineHeight:1.55,fontFamily:"inherit"}}>{generateLvglCode(cfg)}</pre></div>}

          <div style={{marginTop:8,fontSize:9,color:"#3a3a44",display:"flex",gap:10,flexWrap:"wrap"}}>
            <span>Seed {cfg.seed}</span>
            <span>{cfg.width}x{cfg.height}</span>
            <span>{pal.name}</span>
            <span>{cfg.style}</span>
            <span>{shapeCount} obj</span>
            <span>{(FONTS[cfg.titleFont]||FONTS.montserrat).name} {cfg.titleFontSize}px</span>
            {cfg.icons.length>0 && <span>{cfg.icons.length} icon{cfg.icons.length>1?"s":""}</span>}
            <span style={{color:pal.accent}}>0 bytes images</span>
          </div>
        </div>
      </div>
    </div>
  );
}
