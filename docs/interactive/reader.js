'use strict';
const DINO_PIXELS = [
  ".............########...",
  "............##########..",
  "............###.######..",
  "............##########..",
  "............##########..",
  "............######......",
  "............#########...",
  "............#####.......",
  "...........######.......",
  "#.........########......",
  "##.......#########......",
  "###....#############....",
  "####..############.#....",
  "##################......",
  ".#################......",
  "..###############.......",
  "...#############........",
  "....###########.........",
  ".....##########.........",
  "......####.####.........",
  "......###...###.........",
  "......##.....##.........",
  "......###....###........",
  "........................"
];

/* Render the repository's own README so this view does not drift into a
 * second set of setup instructions. All assets and the video stay local. */
const article = document.getElementById('readme');
const mascot = document.getElementById('scroll-dino');
const toggle = document.getElementById('runner-toggle');
const progress = document.querySelector('.reading-progress span');
const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');
const darkMode = matchMedia('(prefers-color-scheme: dark)');

function safeFragment(markdown) {
  const template = document.createElement('template');
  template.innerHTML = marked.parse(markdown, {gfm: true});
  /* The source is this repository, but raw Markdown HTML still does not need
   * executable tags or event handlers to render the documentation. */
  template.content.querySelectorAll('script,iframe,object,embed,form,input,textarea,style,link,meta,base').forEach(node => node.remove());
  template.content.querySelectorAll('*').forEach(node => {
    for (const attribute of [...node.attributes]) {
      if (/^on/i.test(attribute.name)) node.removeAttribute(attribute.name);
      if (['href','src','srcset'].includes(attribute.name) && /^\s*(javascript|vbscript|data):/i.test(attribute.value)) node.removeAttribute(attribute.name);
    }
  });
  return template.content;
}

function addHeadingAnchors() {
  const used = new Map();
  article.querySelectorAll('h1,h2,h3,h4').forEach(heading => {
    const base = heading.textContent.trim().toLowerCase().replace(/[^\p{L}\p{N}\s_-]/gu,'').replace(/\s/g,'-');
    const count = used.get(base) ?? 0;
    heading.id = base + (count ? `-${count}` : '');
    used.set(base, count + 1);
  });
}

function enhanceVideo() {
  const preview = article.querySelector('a[href="demonstration/final_build.mp4"]:has(img)');
  if (!preview) return;
  const video = document.createElement('video');
  video.controls = true;
  video.playsInline = true;
  video.preload = 'metadata';
  video.poster = 'assets/readme/demo-poster.jpg';
  video.setAttribute('aria-label','The 35-second recording of the Dino game running on my STM32 board');
  const source = document.createElement('source');
  source.src = 'demonstration/final_build.mp4';
  source.type = 'video/mp4';
  video.append(source);
  const table = preview.closest('table');
  table.classList.add('demo-table');
  const description = [...table.querySelectorAll('p')].find(node => node.textContent.startsWith('The preview is a short loop'));
  if (description) description.textContent = 'This is the full 35-second recording. Press play to watch the board in action.';
  preview.replaceWith(video);
}

function improveCompanionNote() {
  const heading = document.getElementById('the-little-dino');
  if (!heading) return;
  let node = heading.nextElementSibling;
  while (node && node.tagName !== 'H2') {
    const next = node.nextElementSibling;
    node.remove();
    node = next;
  }
  const explanation = document.createElement('p');
  explanation.textContent = 'You found him. The Dino starts at the top, runs into the margin as you scroll, and keeps you company down the page. Click him to send him home without moving your reading position. Your next scroll brings him back alongside you. The Dino button in the top bar lets him rest.';
  const access = document.createElement('p');
  access.textContent = 'Keyboard users can tab to the Dino and press Enter or Space to send him home. With reduced motion enabled, he uses a still pose and returns home immediately.';
  heading.after(explanation,access);
}

function startDino() {
  const canvas = mascot.querySelector('canvas');
  const context = canvas.getContext('2d');
  context.imageSmoothingEnabled = false;
  const marker = article.querySelector('[data-runner-start]');
  let enabled = true;
  let journey = 'follow', returnLeg = 'rail';
  let x = 0, y = 0, initialized = false;
  let lastTime = performance.now(), lastScrollAt = 0, previousScroll = scrollY;
  let velocity = 0, lastPose = -1, lastInk = '';
  let bounds;

  function measure() {
    const rect = article.getBoundingClientRect();
    const start = marker.getBoundingClientRect();
    const width = mascot.offsetWidth || (innerWidth < 701 ? 38 : 58);
    const height = mascot.offsetHeight || (innerWidth < 701 ? 46 : 66);
    bounds = {
      homeX: start.left + start.width/2 - width/2,
      homeY: start.top + scrollY + Math.max(0,(start.height-height)/2),
      railX: Math.min(document.documentElement.clientWidth-width-5,rect.right + (innerWidth<701 ? 3 : 14)),
      width, height,
      floor: innerHeight-height-28,
      ceiling: document.querySelector('.site-header').getBoundingClientRect().bottom+10
    };
    if (!initialized) {
      x = bounds.homeX;
      y = bounds.homeY-scrollY;
      initialized = true;
    }
  }
  function draw(pose) {
    const ink = getComputedStyle(document.documentElement).getPropertyValue('--ink').trim();
    if (pose === lastPose && ink === lastInk) return;
    context.clearRect(0,0,24,24);
    context.fillStyle = ink;
    const rows = DINO_PIXELS.slice();
    if (pose === 1) {
      rows[20] = '......###...##..........';
      rows[21] = '.......##...###.........';
      rows[22] = '.......###..............';
    }
    rows.forEach((row,r) => [...row].forEach((pixel,c) => {
      if (pixel === '#') context.fillRect(c,r,1,1);
    }));
    lastPose = pose;
    lastInk = ink;
  }
  function sendDinoHome() {
    if (journey === 'home') return;
    if (scrollY < 3) {
      journey = 'home';
      return;
    }
    journey = 'return';
    returnLeg = 'rail';
    velocity = 0;
    mascot.dataset.phase = 'returning';
  }
  mascot.addEventListener('click',sendDinoHome);
  document.addEventListener('click',event => {
    const anchor = event.target.closest('a');
    if (!anchor) return;
    const hash = anchor.getAttribute('href');
    if (!hash?.startsWith('#')) return;
    const target = document.getElementById(decodeURIComponent(hash.slice(1)));
    if (hash === '#top') {
      event.preventDefault();
      window.scrollTo({top:0,behavior:reducedMotion.matches?'auto':'smooth'});
      history.replaceState(null,'',location.pathname+'#top');
    } else if (target) {
      event.preventDefault();
      target.scrollIntoView({behavior:reducedMotion.matches?'auto':'smooth'});
      history.replaceState(null,'',location.pathname+hash);
    }
  });
  toggle.addEventListener('click',() => {
    enabled = !enabled;
    mascot.hidden = !enabled;
    toggle.setAttribute('aria-pressed',String(enabled));
    toggle.textContent = enabled ? (reducedMotion.matches?'Dino: still':'Dino: on') : 'Dino: off';
    if (enabled) measure();
  });
  window.addEventListener('scroll',() => {
    const current = scrollY;
    /* A click only sends the character home. A later scroll invites him back
     * into the margin; the reader's position is never changed by the mascot. */
    if (current !== previousScroll) journey = 'follow';
    velocity = Math.max(-1,Math.min(1,(current-previousScroll)/18));
    previousScroll = current;
    lastScrollAt = performance.now();
  },{passive:true});
  window.addEventListener('resize',measure);
  new ResizeObserver(measure).observe(article);
  reducedMotion.addEventListener('change',() => {
    toggle.textContent = enabled ? (reducedMotion.matches?'Dino: still':'Dino: on') : 'Dino: off';
    lastPose = -1;
    measure();
  });
  darkMode.addEventListener('change',() => { lastInk=''; });
  mascot.hidden = false;
  measure();
  toggle.textContent = reducedMotion.matches?'Dino: still':'Dino: on';

  function animate(now) {
    const dt = Math.min((now-lastTime)/1000,0.06);
    lastTime = now;
    const amount = Math.min(1,dt*11);
    const atTop = scrollY < 3;
    let targetX, targetY;
    if (journey === 'home') {
      targetX = bounds.homeX;
      targetY = bounds.homeY-scrollY;
    } else if (journey === 'return') {
      /* Walk out to the clear margin, then climb it. If his starting slot is
       * above the viewport, finish the trip offscreen rather than moving the
       * document or crossing the text. He waits there until the next scroll. */
      const homeY = bounds.homeY-scrollY;
      const homeVisible = homeY >= bounds.ceiling;
      targetX = returnLeg === 'across' ? bounds.homeX : bounds.railX;
      targetY = returnLeg === 'rail' ? y : homeVisible ? homeY : -bounds.height-8;
    } else if (atTop) {
      targetX = bounds.homeX;
      targetY = bounds.homeY-scrollY;
    } else {
      /* The rail sits outside the article's text rectangle. The first part
       * of the route crosses only the blank mascot slot above the title. */
      const escape = Math.min(1,scrollY/100);
      targetX = bounds.homeX + (bounds.railX-bounds.homeX)*escape;
      const followY = innerHeight*0.66 + velocity*12;
      targetY = scrollY<120 ? bounds.homeY-scrollY : followY;
      targetY = Math.max(bounds.ceiling,Math.min(bounds.floor,targetY));
    }
    if (reducedMotion.matches) {
      if (journey === 'return') journey = 'home';
      const home = journey === 'home' || atTop;
      targetX = home ? bounds.homeX : bounds.railX;
      targetY = home ? bounds.homeY-scrollY : bounds.floor;
    }
    const dx=targetX-x,dy=targetY-y;
    const moving = Math.abs(dx)+Math.abs(dy)>1 || now-lastScrollAt<160;
    x = reducedMotion.matches ? targetX : x+dx*amount;
    /* A steady climbing speed makes the send-home action visible even near
     * the bottom of a long README. Ordinary following uses a soft catch-up. */
    y = reducedMotion.matches ? targetY : journey === 'return' && returnLeg === 'up'
      ? y+Math.sign(dy)*Math.min(Math.abs(dy),dt*460) : y+dy*amount;
    if (journey === 'return' && Math.abs(targetX-x)+Math.abs(targetY-y)<2) {
      if (returnLeg === 'rail') returnLeg = 'up';
      else if (returnLeg === 'up' && bounds.homeY-scrollY >= bounds.ceiling) returnLeg = 'across';
      else journey = 'home';
    }
    if (journey === 'home') {
      x = bounds.homeX;
      y = bounds.homeY-scrollY;
    }
    if (enabled) {
      mascot.style.transform=`translate3d(${x.toFixed(2)}px,${y.toFixed(2)}px,0)`;
      mascot.style.visibility=journey === 'home' && y < bounds.ceiling ? 'hidden' : 'visible';
      if (Math.abs(dx)>1) canvas.style.transform=`scaleX(${dx<0?-1:1})`;
      draw(!reducedMotion.matches && moving ? Math.floor(now/120)%2 : 0);
      mascot.dataset.phase=journey === 'return'?'returning':journey === 'home'||atTop?'home':moving?'running':'idle';
    }
    const total=document.documentElement.scrollHeight-innerHeight;
    progress.style.transform=`scaleX(${total>0?scrollY/total:0})`;
    requestAnimationFrame(animate);
  }
  requestAnimationFrame(animate);
  if (location.hash && location.hash!=='#top') {
    document.getElementById(decodeURIComponent(location.hash.slice(1)))?.scrollIntoView();
  }
}

async function loadReadme() {
  try {
    let markdown=window.LOCAL_README_MARKDOWN;
    if (location.protocol !== 'file:') {
      const response=await fetch('README.md',{cache:'no-store'});
      if (!response.ok) throw new Error(`README returned ${response.status}`);
      markdown=await response.text();
    }
    if (typeof markdown !== 'string') throw new Error('README content is unavailable');
    article.replaceChildren(safeFragment(markdown));
    addHeadingAnchors();
    enhanceVideo();
    improveCompanionNote();
    article.setAttribute('aria-busy','false');
    if (reducedMotion.matches) {
      article.querySelectorAll('img,source').forEach(node => {
        for (const attr of ['src','srcset']) {
          const value=node.getAttribute(attr);
          if (value?.includes('/dino-')) node.setAttribute(attr,value.replace('.gif','.png'));
        }
      });
    }
    startDino();
  } catch(error) {
    article.setAttribute('aria-busy','false');
    article.replaceChildren();
    const heading=document.createElement('h1');
    heading.textContent='The Dino needs a local server.';
    const message=document.createElement('p');
    message.textContent='Run python tools/preview_readme.py from the repository directory, then open the address it prints.';
    article.append(heading,message);
    console.error(error);
  }
}
loadReadme();
