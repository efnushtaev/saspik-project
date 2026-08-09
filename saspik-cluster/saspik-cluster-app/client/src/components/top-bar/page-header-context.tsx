import { createContext, ReactNode, useCallback, useContext, useEffect, useState } from 'react';

export interface PageHeaderContent {
  title: ReactNode;
  actions?: ReactNode;
}

interface PageHeaderContextValue {
  header: PageHeaderContent | null;
  setPageHeader: (content: PageHeaderContent | null) => void;
}

const PageHeaderContext = createContext<PageHeaderContextValue>({
  header: null,
  setPageHeader: () => {},
});

export const PageHeaderProvider = ({ children }: { children: ReactNode }) => {
  const [header, setHeader] = useState<PageHeaderContent | null>(null);
  const setPageHeader = useCallback((content: PageHeaderContent | null) => {
    setHeader(content);
  }, []);

  return (
    <PageHeaderContext.Provider value={{ header, setPageHeader }}>
      {children}
    </PageHeaderContext.Provider>
  );
};

export const usePageHeaderContext = (): PageHeaderContextValue =>
  useContext(PageHeaderContext);

export const usePageHeader = (title: ReactNode | null, actions?: ReactNode | null) => {
  const { setPageHeader } = useContext(PageHeaderContext);

  useEffect(() => {
    const content: PageHeaderContent | null = title || actions ? { title, actions } : null;
    setPageHeader(content);
    return () => setPageHeader(null);
  }, [title, actions, setPageHeader]);
};
