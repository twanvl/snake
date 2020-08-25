"use strict";

function lerp(a,b,t) {
  return (1-t)*a + t*b;
}

// Render a frame
function render(ctx, game, t) {
  let scale = (ctx.canvas.width-2) / game.size[0];
  ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
  ctx.save();
  ctx.translate(1,1);
  
  // position in time
  t = Math.max(0, Math.min(game.snake_pos.length-1, t));
  let ti = Math.floor(t);
  
  // draw grid
  ctx.strokeStyle = dark ? "#666" : "#ccc";
  ctx.lineWidth = 1;
  for (let x=0; x<=game.size[0]; ++x) {
    ctx.lineWidth = x%2 ? 0.8 : 2;
    ctx.beginPath();
    ctx.moveTo(x*scale, 0);
    ctx.lineTo(x*scale, game.size[1]*scale);
    ctx.stroke();
  }
  for (let y=0; y<=game.size[1]; ++y) {
    ctx.lineWidth = y%2 ? 0.8 : 2;
    ctx.beginPath();
    ctx.moveTo(0, y*scale);
    ctx.lineTo(game.size[0]*scale, y*scale);
    ctx.stroke();
  }
  
  // draw Hamiltonian cycle used by agent at time t
  function draw_path(path) {
    ctx.moveTo((path[0][0]+0.5)*scale, (path[0][1]+0.5)*scale);
    for (let i=1; i<path.length; ++i) {
      ctx.lineTo((path[i][0]+0.5)*scale, (path[i][1]+0.5)*scale);
    }
  }
  if (game.cycles) {
    let cycle = game.cycles[ti];
    if (cycle && cycle.length > 0) {
      ctx.beginPath();
      draw_path(cycle);
      ctx.closePath();
      ctx.lineWidth = 0.5 * scale;
      ctx.lineJoin = "round";
      ctx.lineCap = "round";
      ctx.strokeStyle = dark ? "#5555" : "#eee8";
      ctx.stroke();
    }
  }
  
  // draw unreachable cells
  if (game.unreachables) {
    let grid = game.unreachables[ti];
    if (grid && grid.length > 0) {
      ctx.beginPath();
      for (let y=0,i=0; y<game.size[1]; ++y) {
        for (let x=0; x<game.size[0]; ++x,++i) {
          if (lookup_grid(grid, i)) {
            ctx.rect(x*scale+1, y*scale+1, scale-2, scale-2);
            //ctx.rect(x*scale+2, y*scale+2, scale-4, scale-4);
          }
        }
      }
      ctx.fillStyle = "#ff03";
      ctx.fill();
    }
  }
  
  // draw plan used by agent
  if (game.plans) {
    let path = game.plans[ti];
    if (path && path.length > 0) {
      ctx.beginPath();
      draw_path(path);
      ctx.lineWidth = 0.2 * scale;
      ctx.lineJoin = "round";
      ctx.lineCap = "round";
      ctx.strokeStyle = dark ? "#04f5" : "#35f5";
      ctx.stroke();
    }
  }

  // draw apple
  let apple = game.apple_pos_at[ti];
  if (apple) {
    ctx.beginPath();
    ctx.arc((apple[0]+0.5)*scale, (apple[1]+0.5)*scale, 0.4*scale, 0, 2 * Math.PI);
    ctx.fillStyle = dark ? "#f00" : "#e00";
    ctx.fill();
  }
  
  // draw snake
  let pos = function(t) {
    if (t <= 0) return game.snake_pos[0];
    if (t >= game.snake_pos.length) return game.snake_pos[game.snake_pos.length-1];
    let i = Math.max(0, Math.floor(t));
    let a = game.snake_pos[i];
    let b = i+1 < game.snake_pos.length ? game.snake_pos[i+1] : a;
    return [lerp(a[0],b[0],t-i), lerp(a[1],b[1],t-i)];
  }
  let len = game.snake_size[ti];
  if (ti+1 < game.snake_size.length) len = lerp(len,game.snake_size[ti+1],t-ti);
  ctx.beginPath();
  let p = pos(t-(len-1));
  ctx.moveTo((p[0]+0.5)*scale, (p[1]+0.5)*scale);
  for (let i=Math.max(0,Math.ceil(t-(len-1))); i<t; ++i) {
    p = game.snake_pos[i];
    ctx.lineTo((p[0]+0.5)*scale, (p[1]+0.5)*scale);
  }
  p = pos(t);
  ctx.lineTo((p[0]+0.5)*scale, (p[1]+0.5)*scale);
  ctx.strokeStyle = dark ? "#0b0" : "#0a0";
  ctx.lineWidth = 0.4 * scale;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";
  ctx.stroke();
  if (stripes) {
    ctx.setLineDash([1*scale]);
    ctx.lineDashOffset = -len*1*scale;
    ctx.strokeStyle = "#6c0";
    ctx.lineCap = "butt";
    ctx.stroke();
    ctx.setLineDash([]);
  }
  // snake head
  ctx.beginPath();
  ctx.arc((p[0]+0.5)*scale, (p[1]+0.5)*scale, 0.4*scale, 0, 2 * Math.PI);
  ctx.fillStyle = dark ? "#0b0" : "#0a0";
  ctx.fill();
  
  ctx.restore();
}

function render_timeline(ctx, game, t) {
  ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
  let w = ctx.canvas.width, h = ctx.canvas.height;
  let scale = (w-3) / game.snake_pos.length;
  
  // rect
  ctx.beginPath();
  ctx.rect(0.5,4.5,w-1,h-8);
  ctx.strokeStyle = dark ? "#888" : "#ccc";
  ctx.lineWidth = 1;
  ctx.stroke();
  
  // current time
  let x = 1.5 + t * scale;
  ctx.beginPath();
  ctx.moveTo(x,0); ctx.lineTo(x,h);
  ctx.lineWidth = 10;
  ctx.strokeStyle = dark ? "#0b0" : "#0a0";
  ctx.stroke();
  
  // apple times
  ctx.lineWidth = 0.5;
  ctx.strokeStyle = dark ? "#f00" : "#e00";
  for (let e of game.eat_turns) {
    let x = 1.5 + e * scale;
    ctx.beginPath();
    ctx.moveTo(x,5); ctx.lineTo(x,h-4);
    ctx.stroke();
  }
}

function decode_path(data) {
  if (typeof(data) === "string") {
    let x = data.charCodeAt(0)-35;
    let y = data.charCodeAt(1)-35;
    let path = [[x,y]];
    function step(d) {
      if      (d == 0) y--;
      else if (d == 1) y++;
      else if (d == 2) x--;
      else if (d == 3) x++;
      else console.log("invalid path: " + d);
      path.push([x,y]);
    }
    for (let i=2; i<data.length; ++i) {
      let d = data.charCodeAt(i)-35;
      step(d&3);
      d >>= 2;
      if (d) {
        d -= 1;
        step(d&3);
        d >>= 2;
        if (d) {
          d -= 1;
          step(d);
        }
      }
    }
    return path;
  } else {
    return data;
  }
}
function decode_paths(paths, len) {
  for (let i=0; i<len; ++i) {
    if (i >= paths.length || paths[i] === 1) {
      paths[i] = paths[i-1]; // same as previous
    } else if (paths[i] > 0) {
      let diff = paths[i]-1;
      paths[i] = paths[i-1].slice(0, -diff);
    } else if (paths[i] === 0) {
      paths[i] = undefined;
    } else if (typeof(paths[i]) === "string") {
      paths[i] = decode_path(paths[i]);
    }
  }
}
function lookup_grid(data, i) {
  let d = data.charCodeAt(~~(i/6))-35;
  return (d & (1 << (i%6))) != 0;
}
function decode_grids(grids, len) {
  for (let i=0; i<len; ++i) {
    if (i >= grids.length || grids[i] === 1) {
      grids[i] = grids[i-1]; // same as previous
    } else if (grids[i] === 0) {
      grids[i] = undefined;
    }
  }
}
function load_game(data) {
  let game = data;
  if (!game.snake_size) {
    // derive snake length from turns were it eats
    game.snake_size = []
    let l = 1;
    for (let e of game.eat_turns) {
      while (game.snake_size.length <= e) {
        game.snake_size.push(l);
      }
      l++;
    }
  }
  if (!game.apple_pos_at) {
    // derive apple_pos at each time from apple pos when apple is eaten
    game.apple_pos_at = [];
    let p = 0;
    for (let e of game.eat_turns) {
      while (game.apple_pos_at.length < e) {
        game.apple_pos_at.push(data.apple_pos[p]);
      }
      p++;
    }
  }
  game.snake_pos = decode_path(game.snake_pos);
  if (game.cycles) decode_paths(game.cycles, game.snake_pos.length);
  if (game.plans) decode_paths(game.plans, game.snake_pos.length);
  if (game.unreachables) decode_grids(game.unreachables, game.snake_pos.length);
  return game;
}

function init() {
  let canvas = document.getElementById("canvas");
  let ctx = canvas.getContext("2d");
  let agent_lbl = document.getElementById("agent");
  let turn_lbl = document.getElementById("turn");
  let size_lbl = document.getElementById("size");
  let play_lbl = document.getElementById("playpause");
  let speed_lbl = document.getElementById("speed");
  let timeline = document.getElementById("timeline");
  let timeline_ctx = timeline.getContext("2d");

  let game = undefined;
  
  let t = 0;
  let speed = 0.02;
  let prev_timestamp = undefined;
  let playing = false;
  let animFrame;
  
  function update() {
    if (!game) return;
    t = Math.max(0,t);
    t = Math.min(game.snake_pos.length-1,t);
    render(ctx, game, t);
    render_timeline(timeline_ctx, game, t);
    turn_lbl.innerText = "turn " + Math.floor(t) + "/" + (game.snake_pos.length-1);
    size_lbl.innerText = "length " + game.snake_size[Math.floor(t)];
  }

  function anim_step(timestamp) {
    if (!playing) {
      animFrame = undefined;
      return;
    }
    if (prev_timestamp !== undefined) {
      t += (timestamp - prev_timestamp) * speed;
    }
    prev_timestamp = timestamp;
    update();
    // next frame?
    if (t >= game.snake_pos.length-1) {
      pause();
    } else {
      animFrame = requestAnimationFrame(anim_step);
    }
  }
  
  function play() {
    playing = true;
    if (!animFrame) {
      animFrame = requestAnimationFrame(anim_step);
    }
    play_lbl.innerText = "pause";
  }
  function pause() {
    playing = false;
    if (animFrame) {
      cancelAnimationFrame(animFrame);
      animFrame = undefined;
      prev_timestamp = undefined;
    }
    play_lbl.innerText = "play";
  }
  
  function play_pause() {
    if (playing) pause();
    else play();
  }
  
  function seek_left(amount) {
    t = Math.max(0, Math.round(t) - amount);
    update();
  }
  function seek_right(amount) {
    t = Math.min(game.snake_pos.length-1, Math.round(t) + amount);
    update();
  }
  function seek_home() {
    t = 0;
    update();
  }
  function seek_end() {
    t = game.snake_pos.length-1;
    update();
  }
  function update_speed_lbl() {
    speed_lbl.innerText = " (" + Math.round(speed*1000) + " steps/s)"
  }
  function speed_up() {
    speed *= 1.5;
    update_speed_lbl();
  }
  function speed_down() {
    speed /= 1.5;
    update_speed_lbl();
  }
  
  document.getElementById("btn_seek_left").onclick = () => seek_left(10);
  document.getElementById("btn_seek_right").onclick = () => seek_right(10);
  document.getElementById("btn_seek_home").onclick = seek_home;
  document.getElementById("btn_seek_end").onclick = seek_end;
  document.getElementById("btn_speed_up").onclick = speed_up;
  document.getElementById("btn_speed_down").onclick = speed_down;
  document.getElementById("btn_playpause").onclick = play_pause;
  
  document.onkeydown = function(event) {
    if (!game) return;
    if (event.key == ' ') {
      play_pause();
    } else if (event.key == "ArrowLeft") {
      seek_left(event.shiftKey ? 100 : 1);
      event.cancel = true;
    } else if (event.key == "ArrowRight") {
      seek_right(event.shiftKey ? 100 : 1);
      event.cancel = true;
    } else if (event.key == "Home") {
      seek_home();
      event.cancel = true;
    } else if (event.key == "End") {
      seek_end();
      event.cancel = true;
    } else if (event.key == "ArrowUp") {
      speed_up();
      event.cancel = true;
    } else if (event.key == "ArrowDown") {
      speed_down();
      event.cancel = true;
    }
  };
  
  
  
  function seek(event) {
    if (!game) return;
    let x = event.offsetX;
    let w = timeline.width;
    t = (x-1.5) * (game.snake_pos.length-1) / (w-3);
    update();
  }
  
  let button = 0;
  timeline.onmousedown = function(event) {
    seek(event);
    button++;
  };
  timeline.onmouseup = function(event) {
    button = 0;
  };
  timeline.onmousemove = function(event) {
    if (button > 0) seek(event);
  };
  
  function load(data) {
    game = load_game(data);
    window.game = game; // for debugging
    agent_lbl.innerText = "agent: " + game.agent;
    t = 0;
    play();
  }
  
  function unload() {
    game = undefined;
    turn_lbl.innerText = "";
    size_lbl.innerText = "";
    agent_lbl.innerText = "loading...";
  }
  
  // Load json file specified by ?f=.. parameter
  let params = new URLSearchParams(location.search);
  let file = params.get("f");
  if (!file) file = 'examples/cell.json';
  fetch(file)
    .then(response => response.json())
    .then(load)
    .catch(error => console.log(error));
  
  update_speed_lbl();
  unload();
  update();
}

let stripes = false;
let dark = true;

init();
