close all;

## Cubic bezier
##   pts is a vector of complex points p1, p2, p3 and p4
##   t is the bezier interval or a vector thereof
function out = bezier3(pts, t)
  cubic = [
     1  0  0  0
    -3  3  0  0
     3 -6  3  0
    -1  3 -3  1];
    out = zeros(1, length(t));
    for k = 1:length(t)
      kt = t(k);
      out(k) = [1 kt kt*kt kt*kt*kt] * cubic * pts(:);
      ##point = pts(1)*(1-t)^3 + pts(2)*3*(1-t)^2*t + pts(3)*3*(1-t)*t^2 + pts(4)*t^3;
    endfor
endfunction


## Numerical approximation of t for a given x or y value
##   pts is a vector of complex points p1, p2, p3 and p4
##   val is the value to search for or vector thereof
##   xNoty set to nonzero to search for a y value
function t = numerical_t(pts, val, xNoty)
  t = [];
  for v = val
    bound = [0 1];
    for exponent = 1:6
      bt = bound(1) : 1 / power(10, exponent) : bound(2);
      if xNoty == 0
        dat = imag(bezier3(pts, bt));
      else
        dat = real(bezier3(pts, bt));
      endif
      bound(1) = bt(max(find(dat <= v)));
      bound(2) = bt(min(find(dat >= v)));
      if bound(1) == bound(2)
        break
      endif
    endfor
    t = [t (sum(bound) / 2)];
  endfor
endfunction


## Unit vector pointing from p1 -> p2
##   pts is a vector of two points
function uv = unit_vec(pts)
  if length(pts) != 2
    error('pts should be a vector of two complex points')
  endif
  uv = pts(2) - pts(1);
  if uv != 0
    uv = uv / abs(uv);
  else
    warning('Cannot find the unit vec for coincident points');
  endif
endfunction


## 1 - convert a set of cartesian points to linked beziers
## 2 - take p1,p2,p3,p4, plot it, output (x,y) at a set of x values
mode = 2

if mode == 1
  ## x y data
  ref = [
    0	320
    55	309
    110	297
    165	284
    225	269
    285	253
    345	235
    405	217
    475	192
    545	165
    615	131];

  ##ref = [
  ##  0 0
  ##  1 1
  ##  2 0
  ##  3 -1
  ##  4 0];

  ## Convert input data to cartesian points
  dat = complex(ref(:,1), ref(:,2));
  dat_len = length(dat);

  ## For each point k, calculate the average of the gradients
  ## (k-1)->k and k->(k+1)
  grad_vec = zeros(dat_len, 1);
  for k = 1 : length(dat)
    grad_vec(k) = unit_vec([dat(max(k-1, 1)) dat(k)]) + unit_vec([dat(k) dat(min(k+1, dat_len))]);
    grad_vec(k) = grad_vec(k) / abs(grad_vec(k));
  endfor

  beziers = zeros(dat_len-1, 4);
  for k = 1 : length(dat) - 1
    seg_len = abs(dat(k+1) - dat(k));
    beziers(k,1) = dat(k);
    beziers(k,2) = dat(k) + seg_len * cv_length * grad_vec(k);
    beziers(k,3) = dat(k+1) - seg_len * cv_length * grad_vec(k+1);
    beziers(k,4) = dat(k+1);
  endfor

  figure(1);
  hold on;
  axis('equal');
  for k = 1 : length(beziers(:,1))
    plot(real(beziers(k,1)), imag(beziers(k,1)),'o');
    plot(real(beziers(k,4)), imag(beziers(k,4)),'x');
    plot(real(beziers(k,2)), imag(beziers(k,2)),'+');
    plot(real(beziers(k,3)), imag(beziers(k,3)),'*');
    b = zeros(1, 1001);
    pts = beziers(k,:);
    for c = 0 : 1000
      t = c / 1000;
      b(c+1) = bezier3(pts, t);
    endfor
    plot(b, '.')
  endfor
  hold off;

elseif mode == 2
  ref_le = [
    -10 422
    330 354
    660 248
    780 161.6];

  ref_te = [
    -10 25.9
    249 1.5
    457 -10
    780 11.3];

  ref = ref_le
  vals = [-10 0 60 120 180 240 300 360 420 280 515 550 585 620 655 690 725 760 780]
  interp_by = 10

  dat = complex(ref(:,1), ref(:,2));
  figure(1);
  hold on;
  axis('equal');
  plot(real(dat(1)), imag(dat(1)),'o');
  plot(real(dat(4)), imag(dat(4)),'x');
  plot(real(dat(2)), imag(dat(2)),'+');
  plot(real(dat(3)), imag(dat(3)),'*');
  b = zeros(1, 1001);
  for c = 0 : 1000
    t = c / 1000;
    b(c+1) = bezier3(dat, t);
  endfor
  plot(b, '.')
  hold off;

  printf("\n");
  vals = interp1(1:length(vals), vals, 1:(1/interp_by):length(vals));
  t = numerical_t(dat, vals, 1);
  p = bezier3(dat, t);
  f = fopen("curve_points.txt", "w");
  for k = 1:length(p)
    printf("%.1f %.1f\n", real(p(k)), imag(p(k)));
    fprintf(f, "%.1f %.1f\n", real(p(k)), imag(p(k)));
  endfor
  fclose(f);
endif
