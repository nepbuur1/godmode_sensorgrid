// by Marius Versteegen, 2025
// WebGL 3D surface rendering for the Grid visualization page,
// served separately to reduce HTML size.

#pragma once

namespace crt
{
	const char GRID_3D_JS[] = R"rawliteral(
    const GL_VS = `attribute vec3 aPos; attribute vec3 aNor; attribute vec3 aCol;
      uniform mat4 uMVP; varying vec3 vCol; varying vec3 vNor; varying vec3 vPos;
      void main(){ gl_Position = uMVP * vec4(aPos, 1.0); vCol = aCol; vNor = aNor; vPos = aPos; }`;
    const GL_FS = `precision mediump float; varying vec3 vCol; varying vec3 vNor; varying vec3 vPos;
      void main(){
        vec3 n = normalize(vNor);
        vec3 ld = normalize(vec3(1.0, 1.0, 0.0));
        vec3 eye = normalize(vec3(10.0, 10.0, 16.0) - vPos);
        vec3 refl = reflect(-ld, n);
        float diff = max(dot(n, ld), 0.0);
        float spec = pow(max(dot(refl, eye), 0.0), 32.0) * 0.4;
        vec3 c = vCol * (0.3 + 0.7 * diff) + vec3(spec);
        gl_FragColor = vec4(min(c, 1.0), 1.0);
      }`;

    function m4Mul(a, b) {
      const r = new Array(16);
      for (let c = 0; c < 4; c++)
        for (let i = 0; i < 4; i++) {
          let s = 0;
          for (let k = 0; k < 4; k++) s += a[i+k*4]*b[k+c*4];
          r[i+c*4] = s;
        }
      return r;
    }
    function m4Pers(fov, asp, n, f) {
      const t = 1/Math.tan(fov/2), d = n-f;
      return [t/asp,0,0,0, 0,t,0,0, 0,0,(f+n)/d,-1, 0,0,2*f*n/d,0];
    }
    function m4LookAt(ex,ey,ez, cx,cy,cz, ux,uy,uz) {
      let fx=cx-ex, fy=cy-ey, fz=cz-ez;
      let fl=Math.hypot(fx,fy,fz); fx/=fl; fy/=fl; fz/=fl;
      let sx=fy*uz-fz*uy, sy=fz*ux-fx*uz, sz=fx*uy-fy*ux;
      let sl=Math.hypot(sx,sy,sz); sx/=sl; sy/=sl; sz/=sl;
      ux=sy*fz-sz*fy; uy=sz*fx-sx*fz; uz=sx*fy-sy*fx;
      return [sx,ux,-fx,0, sy,uy,-fy,0, sz,uz,-fz,0,
        -(sx*ex+sy*ey+sz*ez),-(ux*ex+uy*ey+uz*ez),(fx*ex+fy*ey+fz*ez),1];
    }

    function initGL(s) {
      const gl = s.surfaceCanvas.getContext("webgl");
      if (!gl) return;
      s.gl = gl;
      const vs = gl.createShader(gl.VERTEX_SHADER);
      gl.shaderSource(vs, GL_VS); gl.compileShader(vs);
      const fs = gl.createShader(gl.FRAGMENT_SHADER);
      gl.shaderSource(fs, GL_FS); gl.compileShader(fs);
      const prog = gl.createProgram();
      gl.attachShader(prog, vs); gl.attachShader(prog, fs);
      gl.linkProgram(prog);
      s.glProg = prog;
      s.glAPos = gl.getAttribLocation(prog, "aPos");
      s.glANor = gl.getAttribLocation(prog, "aNor");
      s.glACol = gl.getAttribLocation(prog, "aCol");
      s.glUMVP = gl.getUniformLocation(prog, "uMVP");
      s.glVtxBuf = gl.createBuffer();
      s.glIdxBuf = gl.createBuffer();
    }

    function colorRGB(v, mn, mx) {
      const lo = normalized ? mn : 0;
      const hi = normalized ? mx : (maxFixed ? getFixedMax() : MAX_VALUE);
      const range = (hi > lo) ? (hi - lo) : 1;
      const t = Math.max(0, Math.min(1, (v - lo) / range));
      if (!colorized) { const g = 0.5 + 0.5*t; return [g, g, g]; }
      const fl = 0.5 * (1 - t);
      let r, g, b;
      if (t < 0.25) { const p=t/0.25; r=0; g=0; b=p; }
      else if (t < 0.5) { const p=(t-0.25)/0.25; r=0; g=p; b=1-p; }
      else if (t < 0.75) { const p=(t-0.5)/0.25; r=p; g=1; b=0; }
      else { const p=(t-0.75)/0.25; r=1; g=1-p; b=0; }
      return [Math.min(1,r+fl), Math.min(1,g+fl), Math.min(1,b+fl)];
    }

    function renderSurface(s, vals, mn, mx) {
      const gl = s.gl;
      if (!gl) return;
      const nC = COLS;
      const nR = Math.ceil(vals.length / nC);
      if (nR < 2 || nC < 2) return;

      let yMax = getPlotMax();
      if (yMax === null || yMax <= 0) yMax = Math.max(1, ...vals);
      const hScale = 5;

      const H = new Float32Array(nR * nC);
      for (let i = 0; i < nR * nC; i++) {
        const v = i < vals.length ? vals[i] : 0;
        H[i] = (v / yMax) * hScale;
      }

      const verts = new Float32Array(nR * nC * 9);
      for (let r = 0; r < nR; r++) {
        for (let c = 0; c < nC; c++) {
          const idx = r * nC + c;
          const v = idx < vals.length ? vals[idx] : 0;
          const rgb = colorRGB(v, mn, mx);
          const hl = c > 0 ? H[idx-1] : H[idx];
          const hr = c < nC-1 ? H[idx+1] : H[idx];
          const hu = r > 0 ? H[idx-nC] : H[idx];
          const hd = r < nR-1 ? H[idx+nC] : H[idx];
          let nx = hl - hr, ny = 2, nz = hu - hd;
          const nl = Math.hypot(nx, ny, nz);
          nx /= nl; ny /= nl; nz /= nl;
          const vi = idx * 9;
          verts[vi]   = c - (nC-1)/2;
          verts[vi+1] = H[idx];
          verts[vi+2] = r - (nR-1)/2;
          verts[vi+3] = nx;
          verts[vi+4] = ny;
          verts[vi+5] = nz;
          verts[vi+6] = rgb[0];
          verts[vi+7] = rgb[1];
          verts[vi+8] = rgb[2];
        }
      }

      if (s.meshRows !== nR || s.meshCols !== nC) {
        const indices = [];
        for (let r = 0; r < nR-1; r++) {
          for (let c = 0; c < nC-1; c++) {
            const tl = r*nC+c, tr = tl+1, bl = tl+nC, br = bl+1;
            indices.push(tl, bl, tr, tr, bl, br);
          }
        }
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, s.glIdxBuf);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);
        s.glIdxCount = indices.length;
        s.meshRows = nR; s.meshCols = nC;
      }

      gl.bindBuffer(gl.ARRAY_BUFFER, s.glVtxBuf);
      gl.bufferData(gl.ARRAY_BUFFER, verts, gl.DYNAMIC_DRAW);

      const cw = s.surfaceCanvas.width, ch = s.surfaceCanvas.height;
      const proj = m4Pers(0.8, cw/ch, 0.1, 100);
      const view = m4LookAt(10, 10, 16, 0, 1, 0, 0, 1, 0);
      const mvp = m4Mul(proj, view);

      gl.viewport(0, 0, cw, ch);
      gl.clearColor(0.067, 0.067, 0.067, 1);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
      gl.enable(gl.DEPTH_TEST);
      gl.useProgram(s.glProg);

      gl.bindBuffer(gl.ARRAY_BUFFER, s.glVtxBuf);
      gl.vertexAttribPointer(s.glAPos, 3, gl.FLOAT, false, 36, 0);
      gl.enableVertexAttribArray(s.glAPos);
      gl.vertexAttribPointer(s.glANor, 3, gl.FLOAT, false, 36, 12);
      gl.enableVertexAttribArray(s.glANor);
      gl.vertexAttribPointer(s.glACol, 3, gl.FLOAT, false, 36, 24);
      gl.enableVertexAttribArray(s.glACol);
      gl.uniformMatrix4fv(s.glUMVP, false, new Float32Array(mvp));

      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, s.glIdxBuf);
      gl.drawElements(gl.TRIANGLES, s.glIdxCount, gl.UNSIGNED_SHORT, 0);
    }

    function toggle3D(id) {
      const s = sensors[id];
      s.is3D = !s.is3D;
      s.gridEl.style.display = s.is3D ? "none" : "";
      s.surfaceCanvas.style.display = s.is3D ? "block" : "none";
      document.getElementById("btn3d" + id).classList.toggle("active", s.is3D);
      if (s.is3D && !s.gl) initGL(s);
      if (s.is3D) colorCells(s, id);
    }
)rawliteral";

} // end namespace crt
