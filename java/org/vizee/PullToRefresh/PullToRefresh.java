package org.vizee.lib;

import android.animation.Animator;
import android.animation.ValueAnimator;
import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.widget.LinearLayout;

public abstract class PullToRefresh extends LinearLayout implements
		ValueAnimator.AnimatorUpdateListener, Animator.AnimatorListener {

	public interface OnPullListener {

		/**
		 * 下拉刷新
		 */
		public void onHeaderRefresh();

		/**
		 * 上拉刷新
		 */
		public void onFooterRefresh();
	}

	protected enum Pulling {
		None, Header, Footer
	}

	protected enum PullState {
		None, Origin, Pull, Tense, Refresh, Contract
	}

	private OnPullListener _onPullListener;
	private boolean _isHeaderEnabled = false;
	private boolean _isFooterEnabled = false;

	private View _header;
	private View _footer;
	private View _content;
	private int _headerHeight = 0;
	private int _footerHeight = 0;

	private int _touchSlop;
	private float _lastTouchY = 0.0f;
	private boolean _isTouchIntercepted = false;
	private Pulling _pulling = Pulling.None;
	private PullState _state = PullState.None;

	private ValueAnimator _scrollYAnimator = null;
	private Runnable _scrollYOnCompleted = null;
	private boolean _isScrollYCancelled = false;

	public PullToRefresh(Context context) {
		super(context);
		initialize(context);
	}

	public PullToRefresh(Context context, AttributeSet attrs) {
		super(context, attrs);
		initialize(context);
	}

	private void initialize(Context context) {
		super.setOrientation(VERTICAL);
		_touchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
		_content = onCreateContent();
		if (_content == null) {
			throw new NullPointerException();
		}
		LayoutParams params = new LayoutParams(LayoutParams.MATCH_PARENT,
				LayoutParams.MATCH_PARENT);
		addView(_content, params);
		setState(PullState.Origin);
	}

	/**
	 * 设置刷新监听器
	 * 
	 * @param listener
	 *            监听器
	 */
	public void setOnPullListener(OnPullListener listener) {
		_onPullListener = listener;
	}

	/**
	 * 获取内容视图
	 * 
	 * @return 内容视图
	 */
	public View getContent() {
		return _content;
	}

	/**
	 * 下拉刷新是否启用
	 * 
	 * @return 是返回true, 否则返回false
	 */
	public boolean isHeaderEnabled() {
		return _isHeaderEnabled;
	}

	/**
	 * 设置下拉刷新是否启用
	 * 
	 * @param enable
	 *            是返回true, 否则返回false
	 */
	public void setHeaderEnable(boolean enable) {
		if (_isHeaderEnabled != enable) {
			if (enable) {
				_header = onCreateHeader();
				if (_header == null) {
					return;
				}
				addView(_header, 0);
				updateHeaderHeight();
				_header.setVisibility(INVISIBLE);
			} else {
				removeView(_header);
				_header = null;
			}
			_isHeaderEnabled = enable;
		}
	}

	/**
	 * 开始下拉刷新
	 */
	public void headerBeginRefresh() {
		if (_pulling != Pulling.Footer && _state != PullState.Refresh) {
			setPulling(Pulling.Header);
			setState(PullState.Refresh);
			scrollYTo(-_headerHeight, new Runnable() {

				@Override
				public void run() {
					OnPullListener listener = _onPullListener;
					if (_state == PullState.Refresh && listener != null) {
						listener.onHeaderRefresh();
					}
				}
			});
		}
	}

	/**
	 * 下拉刷新完成
	 */
	public void headerEndRefresh() {
		if (_pulling != Pulling.Header) {
			return;
		}
		setState(PullState.Contract);
		scrollYTo(0, new Runnable() {

			@Override
			public void run() {
				if (_header != null) {
					_header.setVisibility(INVISIBLE);
				}
				setState(PullState.Origin);
				setPulling(Pulling.None);
			}
		});
	}

	/**
	 * 上拉刷新是否启用
	 * 
	 * @return 是返回true, 否则返回false
	 */
	public boolean isFooterEnabled() {
		return _isFooterEnabled;
	}

	/**
	 * 设置上拉刷新是否启用
	 * 
	 * @param enable
	 *            是返回true, 否则返回false
	 */
	public void setFooterEnable(boolean enable) {
		if (_isFooterEnabled != enable) {
			if (enable) {
				_footer = onCreateFooter();
				if (_footer == null) {
					return;
				}
				addView(_footer, -1);
				updateFooterHeight();
				_footer.setVisibility(INVISIBLE);
			} else {
				removeView(_footer);
				_footer = null;
			}
			_isFooterEnabled = enable;
		}
	}

	/**
	 * 开始上拉刷新
	 */
	public void footerBeginRefresh() {
		if (_pulling != Pulling.Header && _state != PullState.Refresh) {
			setPulling(Pulling.Footer);
			setState(PullState.Refresh);
			scrollYTo(_footerHeight, new Runnable() {

				@Override
				public void run() {
					OnPullListener listener = _onPullListener;
					if (_state == PullState.Refresh && listener != null) {
						listener.onFooterRefresh();
					}
				}
			});
		}
	}

	/**
	 * 结束上拉刷新
	 */
	public void footerEndRefresh() {
		if (_pulling != Pulling.Footer) {
			return;
		}
		setState(PullState.Contract);
		scrollYTo(0, new Runnable() {

			@Override
			public void run() {
				if (_footer != null) {
					_footer.setVisibility(INVISIBLE);
				}
				setState(PullState.Origin);
				setPulling(Pulling.None);
			}
		});
	}

	@Override
	public final void setOrientation(int orientation) {
		throw new UnsupportedOperationException();
	}

	@Override
	public boolean onInterceptTouchEvent(MotionEvent ev) {
		if (!_isHeaderEnabled && !_isFooterEnabled) {
			return false;
		}
		int action = ev.getAction();
		if (action != MotionEvent.ACTION_DOWN && _isTouchIntercepted) {
			return true;
		}
		switch (action) {
		case MotionEvent.ACTION_DOWN:
			_lastTouchY = ev.getY();
			_isTouchIntercepted = false;
			break;
		case MotionEvent.ACTION_MOVE:
			float dy = ev.getY() - _lastTouchY;
			if (Math.abs(dy) < _touchSlop) {
				break;
			}
			if (_pulling != Pulling.Footer && _isHeaderEnabled
					&& isHeaderPullable()) {
				_isTouchIntercepted = (_pulling == Pulling.Header || (_pulling == Pulling.None && dy > 0));
				if (_isTouchIntercepted) {
					if (_pulling == Pulling.Header) {
						cancelScollY();
					} else {
						setPulling(Pulling.Header);
					}
				}
			}
			if (_pulling != Pulling.Header && _isFooterEnabled
					&& isFooterPullable()) {
				_isTouchIntercepted = (_pulling == Pulling.Footer || (_pulling == Pulling.None && dy < 0));
				if (_isTouchIntercepted) {
					if (_pulling == Pulling.Footer) {
						cancelScollY();
					} else {
						setPulling(Pulling.Footer);
						_footer.setVisibility(VISIBLE);
					}
				}
			}
			if (_isTouchIntercepted) {
				onTouchEvent(ev);
			}
			break;
		default:
			break;
		}
		return _isTouchIntercepted;
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		boolean handled = false;
		switch (event.getAction()) {
		case MotionEvent.ACTION_DOWN:
			_lastTouchY = event.getY();
			_isTouchIntercepted = false;
			break;
		case MotionEvent.ACTION_MOVE:
			float y = event.getY();
			float dy = y - _lastTouchY;
			_lastTouchY = y;
			if (_pulling == Pulling.Header) {
				pullHeader(dy);
				handled = true;
			} else if (_pulling == Pulling.Footer) {
				pullFooter(dy);
				handled = true;
			} else {
				scrollYTo(0, null);
				_isTouchIntercepted = false;
			}
			break;
		case MotionEvent.ACTION_CANCEL:
		case MotionEvent.ACTION_UP:
			if (_isTouchIntercepted) {
				boolean refreshing = false;
				if (event.getAction() != MotionEvent.ACTION_CANCEL
						&& _state == PullState.Tense) {
					if (_pulling == Pulling.Header) {
						refreshing = true;
						headerBeginRefresh();
					} else if (_pulling == Pulling.Footer) {
						refreshing = true;
						footerBeginRefresh();
					}
				}
				if (!refreshing) {
					if (_pulling == Pulling.Header) {
						headerEndRefresh();
					} else if (_pulling == Pulling.Footer) {
						footerEndRefresh();
					}
				}
				_isTouchIntercepted = false;
			}
			break;
		default:
			break;
		}
		return handled;
	}

	@Override
	public void onAnimationUpdate(ValueAnimator animation) {
		setScrollY((Integer) animation.getAnimatedValue());
	}

	protected void updateHeaderHeight() {
		if (_header == null) {
			return;
		}
		_headerHeight = onUpdateHeaderHeight();
		LayoutParams params = (LayoutParams) _header.getLayoutParams();
		if (params != null) {
			params.height = _headerHeight;
			params.topMargin = -_headerHeight;
			_header.requestLayout();
		}
	}

	protected void updateFooterHeight() {
		if (_footer == null) {
			return;
		}
		_footerHeight = onUpdateFooterHeight();
		LayoutParams params = (LayoutParams) _footer.getLayoutParams();
		if (params != null) {
			params.height = _footerHeight;
			params.bottomMargin = -_footerHeight;
			_footer.requestLayout();
		}
	}

	private void setPulling(Pulling pulling) {
		if (_pulling == pulling) {
			return;
		}
		switch (_pulling) {
		case Header:
			_header.setVisibility(INVISIBLE);
			break;
		case Footer:
			_footer.setVisibility(INVISIBLE);
			break;
		default:
			break;
		}
		_pulling = pulling;
		switch (_pulling) {
		case Header:
			if (_header != null) {
				_header.setVisibility(VISIBLE);
			}
			break;
		case Footer:
			if (_footer != null) {
				_footer.setVisibility(VISIBLE);
			}
			break;
		default:
			break;
		}
	}

	private void setState(PullState state) {
		if (_state != state) {
			_state = state;
			onStateChange(_pulling, state);
		}
	}

	private void pullHeader(float offset) {
		int y = getScrollY() - (int) (offset / 1.618f);
		if (y >= 0) {
			y = 0;
		}
		setScrollY(y);
		if (-y >= _headerHeight) {
			setState(PullState.Tense);
		} else if (_state != PullState.Refresh) {
			setState(PullState.Pull);
		}
	}

	private void pullFooter(float offset) {
		int y = getScrollY() - (int) (offset / 1.618f);
		if (y <= 0) {
			y = 0;
		}
		setScrollY(y);
		if (y >= _footerHeight) {
			setState(PullState.Tense);
		} else if (_state != PullState.Refresh) {
			setState(PullState.Pull);
		}
	}

	private void cancelScollY() {
		if (_scrollYAnimator != null) {
			_scrollYAnimator.cancel();
		}
	}

	private void scrollYTo(int y, Runnable onCompleted) {
		cancelScollY();
		_isScrollYCancelled = false;
		_scrollYOnCompleted = onCompleted;
		_scrollYAnimator = ValueAnimator.ofInt(getScrollY(), y);
		_scrollYAnimator.setDuration(Math.round(Math.sqrt(Math.abs(y
				- getScrollY()))) * 13 + 10);
		_scrollYAnimator.addUpdateListener(this);
		_scrollYAnimator.addListener(this);
		_scrollYAnimator.start();
	}

	@Override
	public void onAnimationStart(Animator animation) {
	}

	@Override
	public void onAnimationRepeat(Animator animation) {
	}

	@Override
	public void onAnimationEnd(Animator animation) {
		if (!_isScrollYCancelled && _scrollYOnCompleted != null) {
			_scrollYOnCompleted.run();
		}
		_scrollYAnimator = null;
		_scrollYOnCompleted = null;
	}

	@Override
	public void onAnimationCancel(Animator animation) {
		_isScrollYCancelled = true;
	}

	protected abstract View onCreateContent();

	protected void onStateChange(Pulling pulling, PullState state) {
	}

	protected View onCreateHeader() {
		return null;
	}

	protected int onUpdateHeaderHeight() {
		return 0;
	}

	protected boolean isHeaderPullable() {
		return false;
	}

	protected View onCreateFooter() {
		return null;
	}

	protected int onUpdateFooterHeight() {
		return 0;
	}

	protected boolean isFooterPullable() {
		return false;
	}
}
