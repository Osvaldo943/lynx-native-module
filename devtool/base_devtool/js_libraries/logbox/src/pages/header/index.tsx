// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { getBridge } from '@/jsbridge';
import type { IViewsInfoState } from '@/models/viewsInfo';
import { toggleURLDisplay } from '@/models/viewsInfo';
import styles from './index.less';
import React, { useState } from 'react';
import vConsole from '@/utils/vconsole';
import { CloseOutlined, DeleteOutlined, InfoCircleOutlined } from '@ant-design/icons';
import { clearErrors, clearErrorCache } from '@/models/errorReducer';
import { useDispatch, useSelector } from 'react-redux';

function HeaderImpl(): JSX.Element {
  const { currentView, viewsCount, level, templateUrl } = useSelector((state) => state.viewsInfo);
  const [clickedTimes, updateClickedTimes] = useState(0);
  const dispatch = useDispatch();

  if (clickedTimes > 5) {
    vConsole.showSwitch();
  }

  const clickCallback = () => {
    updateClickedTimes(clickedTimes + 1);
  };

  const clearErrorAndEntry = () => {
    dispatch(clearErrors());
    dispatch(clearErrorCache());
  };

  const previous = () => {
    const number = currentView - 1 < 1 ? viewsCount : currentView - 1;
    clearErrorAndEntry();
    getBridge().changeView(number);
  };

  const next = () => {
    const number = currentView + 1 <= viewsCount ? currentView + 1 : 1;
    clearErrorAndEntry();
    getBridge().changeView(number);
  };

  return (
    <div className={styles.container}>
      <div
        className={`${styles.navigation} ${level === 'error' ? styles.error : styles.warning}`}
        style={{ height: '40px', display: viewsCount > 1 ? 'flex' : 'none' }}
      >
        <button
          onClick={previous}
          className={`${styles.navButton} ${styles.navButtonLeft} ${level === 'error' ? styles.navButtonError : styles.navButtonWarning}`}
        >
          PREVIOUS
        </button>
        <span className={styles.counter}>{`${currentView} / ${viewsCount} `}</span>
        <button
          onClick={next}
          className={`${styles.navButton} ${styles.navButtonRight} ${level === 'error' ? styles.navButtonError : styles.navButtonWarning}`}
        >
          NEXT
        </button>
      </div>
      <div className={styles.header}>
        <img
          className={styles.icon}
          src="./LynxIcon.svg"
          onClick={() => {
            clickCallback();
          }}
        ></img>
        <span className={styles.title}>Lynx </span>
        <span className={styles.expand}></span>
        {!!templateUrl && (
          <InfoCircleOutlined
            className={styles.buttonIcon}
            onClick={() => {
              dispatch(toggleURLDisplay());
            }}
          />
        )}
        <span className={styles.gap}></span>
        <DeleteOutlined
          className={styles.buttonIcon}
          onClick={() => {
            clearErrorAndEntry();
            getBridge().deleteCurrentView(currentView);
          }}
        />
        <span className={styles.gap}></span>
        <CloseOutlined
          className={styles.buttonIcon}
          onClick={() => {
            clearErrorAndEntry();
            getBridge().dismissError();
          }}
        />
      </div>
    </div>
  );
}

export const Header = HeaderImpl;
