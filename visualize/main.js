"use strict";

function lerp(a,b,t) {
  return (1-t)*a + t*b;
}

// Render a frame
function render(ctx, game, t) {
  let scale = ctx.canvas.width / game.size[0];
  ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
  
  // draw grid
  ctx.beginPath();
  for (let x=0; x<=game.size[0]; ++x) {
    ctx.moveTo(x*scale, 0);
    ctx.lineTo(x*scale, game.size[1]*scale);
  }
  for (let y=0; y<=game.size[1]; ++y) {
    ctx.moveTo(0, y*scale);
    ctx.lineTo(game.size[0]*scale, y*scale);
  }
  ctx.strokeStyle = "#ccc";
  ctx.lineWidth = 1;
  ctx.stroke();
  
  // position in time
  t = Math.min(t, game.snake_pos.length-1);
  let ti = Math.max(0, Math.floor(t));
  
  // draw apple
  {
    let apple = game.apple_pos[ti];
    ctx.beginPath();
    ctx.arc((apple[0]+0.5)*scale, (apple[1]+0.5)*scale, 0.4*scale, 0, 2 * Math.PI);
    ctx.fillStyle = "#e00";
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
  let p = pos(t-len);
  ctx.moveTo((p[0]+0.5)*scale, (p[1]+0.5)*scale);
  for (let i=Math.max(0,Math.ceil(t-len)); i<t; ++i) {
    p = game.snake_pos[i];
    ctx.lineTo((p[0]+0.5)*scale, (p[1]+0.5)*scale);
  }
  p = pos(t);
  ctx.lineTo((p[0]+0.5)*scale, (p[1]+0.5)*scale);
  ctx.strokeStyle = "#0a0";
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
  ctx.fillStyle = "#0a0";
  ctx.fill();
}

let stripes = false;

let canvas = document.getElementById("canvas");
let ctx = canvas.getContext("2d");
//render(ctx, game, 0);
//render(ctx, game, 100.5);

let t = 100;
//let speed = 0.01;
let speed = 0.005;
var prev_timestamp = undefined;
function render_step(timestamp) {
  //let speed = 0.01 + t/1000;
  if (prev_timestamp !== undefined) {
    t += (timestamp - prev_timestamp) * speed;
  }
  prev_timestamp = timestamp;
  render(ctx, game, t);
  requestAnimationFrame(render_step);
}
requestAnimationFrame(render_step);

